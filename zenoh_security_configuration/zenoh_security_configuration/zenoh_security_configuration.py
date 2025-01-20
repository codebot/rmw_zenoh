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
import em
from io import StringIO
import os
import pathlib
import sys
from ament_index_python.packages import get_package_share_directory

class ZenohSecutiryConfigGenerator:
    """ZenohSecutiryConfigGenerator generates Zenoh secutiry configurations."""

    def __init__(self):
        """
        Construct ZenohConfigGenerator.
        """
        self.zenoh_cfg_file_extension = "json5"

    def generate_router_config(self, data, zenoh_type):
        try:
            output = StringIO()
            interpreter = em.Interpreter(
                output=output,
                options={
                    em.BUFFERED_OPT: True,
                    em.RAW_OPT: True,
                },
            )

            template_path = pathlib.Path(os.path.join(
                get_package_share_directory("zenoh_security_configuration"),
                "templates",
                zenoh_type + ".json5"
            ))

            with template_path.open('r') as h:
                template_content = h.read()
            interpreter.string(template_content, locals=data)
            return output.getvalue()
        except Exception as e:
            print(e)
        finally:
            if interpreter is not None:
                interpreter.shutdown()
            interpreter = None

    def generate_zenoh_config(self, output_dir, router_config, zenoh_type, encoding: str = 'utf-8'):
        """
        Generate Zenoh bridge configs and output to directory 'output_dir'.

        Parameters
        ----------
        output_dir : str
            Output directory for Zenoh configurations

        """
        write_filepath = os.path.join(
            output_dir, zenoh_type + "."
            + self.zenoh_cfg_file_extension,
        )
        output_file = pathlib.Path(write_filepath)
        print(f"Generated Zenoh secutiry configuration at {write_filepath}")
        if output_file.exists():
            existing_content = output_file.read_text(encoding=encoding)
            if existing_content == router_config:
                return
        elif output_file.parent:
            os.makedirs(str(output_file.parent), exist_ok=True)

        output_file.write_text(router_config, encoding=encoding)

def main(argv=sys.argv):
    """Entrypoint."""
    parser = argparse.ArgumentParser(
        description="Generate Zenoh security configurations",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    subparsers = parser.add_subparsers(help='help for subcommand', dest="subcommand")
    parser_path = subparsers.add_parser('paths', help='Use path')
    parser_enclave = subparsers.add_parser('enclave', help='Use enclave directory')

    parser.add_argument(
        "-o",
        "--output",
        required=True,
        type=str,
        help="Output directory for Zenoh bridge configurations",
    )

    parser.add_argument(
        "-l",
        "--listen_endpoint",
        required=False,
        type=str,
        default ="tls/localhost:7447",
        help="The list of endpoints to listen on (See https://docs.rs/zenoh/latest/zenoh/config/struct.EndPoint.html)",
    )

    parser.add_argument(
        "-c",
        "--connect_endpoint",
        required=False,
        type=str,
        default ="tls/localhost:7447",
        help="The list of endpoints to connect to. (See https://docs.rs/zenoh/latest/zenoh/config/struct.EndPoint.html)",
    )

    parser.add_argument(
        "-p",
        "--protocols",
        nargs='*',
        required=False,
        choices=["tcp", "tls"],
        default=['tls'],
        help="Protocols chooices",
    )

    parser.add_argument(
        "-t",
        "--type",
        type=str,
        required=True,
        choices=["router", "peer"],
        default=['router'],
        help="Set router or peer",
    )

    parser_path.add_argument(
        "--root_ca_certificate",
        type=str,
        required=True,
        default='',
        help="Path to the certificate of the certificate authority used to validate " \
            "either the server or the client's keys and certificates"
    )

    parser_path.add_argument(
        "--listen_private_key",
        type=str,
        required=True,
        default='',
        help="Path to the TLS listening side private key"
    )

    parser_path.add_argument(
        "--listen_certificate",
        type=str,
        required=True,
        default='',
        help="Path to the TLS listening side public certificate"
    )

    parser_path.add_argument(
        "--connect_private_key",
        type=str,
        required=True,
        default='',
        help="Path to the TLS connecting side private key"
    )

    parser_path.add_argument(
        "--connect_certificate",
        type=str,
        required=True,
        default='',
        help="Path to the TLS connecting side certificate"
    )

    parser_enclave.add_argument(
        "--enclave_path",
        type=str,
        required=True,
        default='',
        help="Enclave path"
    )

    parser_enclave.add_argument(
        "--enclave_name",
        type=str,
        required=True,
        default='',
        help="Enclave name"
    )

    args = parser.parse_args(argv[1:])

    if args.enclave_name is not None:
        if args.enclave_name[0] == '/':
            args.enclave_name = args.enclave_name[1:]
        root_ca_certificate = os.path.join(args.enclave_path, "public", "ca.cert.pem")
        listen_private_key = os.path.join(args.enclave_path, "enclaves", args.enclave_name, "key.pem")
        listen_certificate = os.path.join(args.enclave_path, "enclaves", args.enclave_name, "cert.pem")
        connect_private_key = listen_private_key
        connect_certificate = listen_certificate
    else:
        root_ca_certificate = args.root_ca_certificate
        listen_private_key = args.listen_private_key
        listen_certificate = args.listen_certificate
        connect_private_key = args.connect_private_key
        connect_certificate = args.connect_certificate

    data = {
        'protocols': args.protocols,
        'listen_endpoint': args.listen_endpoint,
        'connect_endpoint': args.connect_endpoint,
        'root_ca_certificate': root_ca_certificate,
        'listen_private_key': listen_private_key,
        'listen_certificate': listen_certificate,
        'connect_private_key': connect_private_key,
        'connect_certificate': connect_certificate,
    }
    zenoh_sec_gen = ZenohSecutiryConfigGenerator()
    router_config = zenoh_sec_gen.generate_router_config(data, args.type)
    zenoh_sec_gen.generate_zenoh_config(args.output, router_config, args.type)

if __name__ == "__main__":
    main(sys.argv)
