# Copyright 2025 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
from io import StringIO
import os
import pathlib
import sys

from ament_index_python.packages import get_package_share_directory

import zenoh
import json
import json5

import xml.etree.ElementTree as ET

import em


class ZenohSecutiryConfigGenerator:
    """ZenohSecutiryConfigGenerator generates Zenoh secutiry configurations."""

    def __init__(self, config_file_path, policy_path):
        """Construct ZenohConfigGenerator."""

        self.config_file_path = config_file_path

        self.policy_tree = ET.parse(policy_path)
        self.policy_root = self.policy_tree.getroot()

    def get_enclaves(self):
        for enclaves in self.policy_root.iter('enclaves'):
            for enclave in enclaves.iter('enclave'):
                self.get_profiles(enclave)

    def get_profiles(self, enclave):
        for profiles in enclave.iter('profiles'):
            for profile in profiles.iter('profile'):
                node_name = profile.attrib['node']
                conf = (
                    zenoh.Config.from_file(self.config_file_path)
                    if self.config_file_path is not None
                    else zenoh.Config()
                )

                conf.insert_json5('access_control/enabled', 'true')
                conf.insert_json5('access_control/default_permission', "'deny'")

                services_reply_allow, services_reply_deny, services_request_allow, services_request_deny = self.get_services(profile, node_name)
                topics_sub_allow, topics_pub_allow, topics_sub_deny, topics_pub_deny = self.get_topics(profile)
                rules = self.generate_rules(node_name,
                                            services_reply_allow, services_reply_deny,
                                            services_request_allow, services_request_deny,
                                            topics_sub_allow, topics_pub_allow,
                                            topics_sub_deny, topics_pub_deny)
                has_services = len(services_reply_allow) > 0 or len(services_request_allow) > 0
                rules = self.generate_liveliness(rules, has_services)
                conf.insert_json5('access_control/rules', json.dumps(rules))

                policies = self.set_policies(rules, node_name)
                subjects = self.generate_subjects(node_name)
                conf.insert_json5('access_control/policies', json.dumps(policies))
                conf.insert_json5('access_control/subjects', json.dumps(subjects))
                # print(conf)
                lol = json5.loads(str(conf))
                # print(conf)
                print(json5.dumps(lol))
                # print(json5.dumps(str(conf), sort_keys=True))
                with open(node_name + '.json5', 'w', encoding='utf-8') as f:
                    # json5.dump(str(conf), f)
                    f.write(str(conf))

    def check_service_name(self, service_name, node_name):
        if service_name[0] == '~':
            service_name = service_name.replace('~', node_name)
        return service_name

    def get_services(self, profile, node_name):
        services_reply_allow = set()
        services_reply_deny = set()
        services_request_allow = set()
        services_request_deny = set()
        for services in profile.iter('services'):
            service_type = list(services.attrib.keys())[0]
            permission = list(services.attrib.values())[0]
            for service in services.iter('service'):
                if service_type == 'reply':
                    if permission == 'ALLOW':
                        services_reply_allow.add(self.check_service_name(service.text, node_name))
                    if permission == 'DENY':
                        services_reply_deny.add(self.check_service_name(service.text, node_name))
                elif service_type == 'request':
                    if permission == 'ALLOW':
                        services_request_allow.add(self.check_service_name(service.text, node_name))
                    if permission == 'DENY':
                        services_request_deny.add(self.check_service_name(service.text, node_name))

        return [services_reply_allow, services_reply_deny,
                services_request_allow, services_request_deny]

    def get_topics(self, profile):
        topics_sub_allow = set()
        topics_pub_allow = set()
        topics_sub_deny = set()
        topics_pub_deny = set()
        for topics in profile.iter('topics'):
            for topic in topics.iter('topic'):
                topic_type = list(topics.attrib.keys())[0]
                permission = list(topics.attrib.values())[0]
                if topic_type == 'subscribe':
                    if permission == 'ALLOW':
                        topics_sub_allow.add(topic.text)
                    if permission == 'DENY':
                        topics_sub_deny.add(topic.text)
                elif topic_type == 'publish':
                    if permission == 'ALLOW':
                        topics_pub_allow.add(topic.text)
                    if permission == 'DENY':
                        topics_pub_deny.add(topic.text)
        return [topics_sub_allow, topics_pub_allow, topics_sub_deny, topics_pub_deny]

    def generate_subjects(self, id):
        subjects = [
            {'id': 'router'},
            {'id': id}
        ]
        return subjects

    def generate_rules(self, node_name,
                       services_reply_allow, services_reply_deny,
                       services_request_allow, services_request_deny,
                       topics_sub_allow, topics_pub_allow,
                       topics_sub_deny, topics_pub_deny):
        rules = []

        if len(services_reply_allow) > 0:
            incoming_queries = {
                "id": "incoming_queries",
                "messages": ["query" ],
                "flows":["ingress"],
                "permission": "allow",
                "key_exprs":
                    [f"0/{service}/**" for service in services_reply_allow]
            }
            rules.append(incoming_queries)

            outgoing_queryables_replies = {
                "id": "outgoing_queryables_replies",
                "messages": ["declare_queryable", "reply" ],
                "flows":["egress"],
                "permission": "allow",
                "key_exprs":
                    [f"0/{service}/**" for service in services_reply_allow]
            }
            rules.append(outgoing_queryables_replies)

        if len(services_request_allow) > 0:
            outgoing_queries = {
                "id": "outgoing_queries",
                "messages": ["query" ],
                "flows":["egress"],
                "permission": "allow",
                "key_exprs": [f"0/{service}/**" for service in services_request_allow]
            }
            rules.append(outgoing_queries)


        if len(topics_pub_allow) != 0:
            outgoing_publications = {
                'id': 'outgoing_publications',
                'messages': [ 'put' ],
                'flows':['egress'],
                'permission': 'allow',
                'key_exprs': [f"0/{topic}/**" for topic in topics_pub_allow],
            }
        rules.append(outgoing_publications)

        if len(topics_sub_allow) != 0:
            outgoing_subscriptions = {
                "id": "outgoing_subscriptions",
                "messages": [ "declare_subscriber" ],
                "flows":["egress"],
                "permission": "allow",
                "key_exprs": [f"0/{topic}/**" for topic in topics_sub_allow]
            }
            rules.append(outgoing_subscriptions)

        if len(topics_pub_allow) != 0:
            incoming_subscriptions ={
                'id': 'incoming_subscriptions',
                'messages': [ 'declare_subscriber' ],
                'flows':['ingress'],
                'permission': 'allow',
                'key_exprs': [f"0/{topic}/**" for topic in topics_pub_allow]
            }
        if len(topics_sub_allow) != 0:
            incoming_publications =  {
                "id": "incoming_publications",
                "messages": [ "put" ],
                "flows":["ingress"],
                "permission": "allow",
                "key_exprs": [f"0/{topic}/**" for topic in topics_sub_allow],
            }
            rules.append(incoming_subscriptions)
            rules.append(incoming_publications)
        return rules

    def generate_liveliness(self, rules, has_services):

        messages = ['liveliness_token', 'liveliness_query', 'declare_liveliness_subscriber' ]

        if has_services:
            messages.append('reply')

        liveliness = {
            'id': 'liveliness_tokens',
            'messages': messages,
            'flows':['ingress', 'egress'],
            'permission': 'allow',
            'key_exprs': [ '@ros2_lv/0/**' ],
        }
        rules.append(liveliness)
        return rules

    def set_policies(self, rules, node_name):
        r = []
        for rule in rules:
            r.append(rule['id'])

        policies = [
            {
                "rules": ['liveliness_tokens'],
                "subjects": ['router']
            },
            {
                "rules": r,
                "subjects": [node_name]
            }
        ]
        return policies


    def generate_config_files(self):
        self.get_enclaves()


def main(argv=sys.argv):
    """Entrypoint."""
    parser = argparse.ArgumentParser(
        description='Generate Zenoh security configurations',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument(
        '--policy',
        required=False,
        type=str,
        help='Policy file',
    )

    parser.add_argument(
        '--config',
        required=False,
        type=str,
        help='Zenoh config file',
    )

    args = parser.parse_args(argv[1:])

    zscg = ZenohSecutiryConfigGenerator(args.config, args.policy)
    zscg.generate_config_files()

if __name__ == '__main__':
    main(sys.argv)
