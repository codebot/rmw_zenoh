# zenoh security configuration

### Configure the router

```bash
ros2 run zenoh_security_configuration zenoh_security_configuration \
    -o zenoh_config \
    -t router \
    --listen_endpoint="tls/localhost:7447" \
    --protocols=tls \
    paths \
    --root_ca_certificate /home/ahcorde/sros2_demo/demo_keystore_zenoh/public/ca.cert.pem \
    --listen_private_key /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/zenohd/key.pem \
    --connect_private_key /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/zenohd/key.pem \
    --connect_certificate /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/zenohd/cert.pem \
    --listen_certificate /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/zenohd/cert.pem
```

Using enclaves generated with `ros2 security create_enclave`

```bash
ros2 run zenoh_security_configuration zenoh_security_configuration \
    -o zenoh_config \
    -t router \
    --listen_endpoint="tls/localhost:0" \
    --protocols=tls \
    enclave \
    --enclave_path /home/ahcorde/sros2_demo/demo_keystore_zenoh \
    --enclave_name /zenoh
```

### Configure a Peer

```bash
ros2 run zenoh_security_configuration zenoh_security_configuration \
    -o zenoh_config \
    -t peer \
    --listen_endpoint="tls/localhost:0" \
    --protocols=tls \
    paths \
    --root_ca_certificate /home/ahcorde/sros2_demo/demo_keystore_zenoh/public/ca.cert.pem \
    --listen_private_key /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/listener/key.pem \
    --connect_private_key /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/listener/key.pem \
    --connect_certificate /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/listener/cert.pem \
    --listen_certificate /home/ahcorde/sros2_demo/demo_keystore_zenoh/enclaves/listener/cert.pem
```

Using enclaves generated with `ros2 security create_enclave`


```bash
ros2 run zenoh_security_configuration zenoh_security_configuration \
    -o zenoh_config \
    -t peer \
    --listen_endpoint="tls/localhost:0" \
    --protocols=tls \
    enclave \
    --enclave_path /home/ahcorde/sros2_demo/demo_keystore_zenoh \
    --enclave_name /talker_listener/talker
```
