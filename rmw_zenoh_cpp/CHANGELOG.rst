^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_zenoh_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.1.1 (2025-02-04)
------------------
* Bump Zenoh to commit id e4ea6f0 (1.2.0 + few commits) (`#449 <https://github.com/ros2/rmw_zenoh/issues/449>`_)
* Inform users that peers will not discover and communicate with one another until the router is started (`#445 <https://github.com/ros2/rmw_zenoh/issues/445>`_)
* Clear the error after rmw_serialized_message_resize() (`#437 <https://github.com/ros2/rmw_zenoh/issues/437>`_)
* Fix `ZENOH_ROUTER_CHECK_ATTEMPTS` which was not respected (`#429 <https://github.com/ros2/rmw_zenoh/issues/429>`_)
* fix: use the default destructor to drop the member `Payload` (`#421 <https://github.com/ros2/rmw_zenoh/issues/421>`_)
* Remove `gid_hash\_` from `AttachmentData` (`#418 <https://github.com/ros2/rmw_zenoh/issues/418>`_)
* Sync the config with the default config in Zenoh. (`#414 <https://github.com/ros2/rmw_zenoh/issues/414>`_)
* fix: check the context validity before accessing the session (`#407 <https://github.com/ros2/rmw_zenoh/issues/407>`_)
* Fix wan't typo (`#402 <https://github.com/ros2/rmw_zenoh/issues/402>`_)
* Contributors: Alejandro Hernández Cordero, ChenYing Kuo (CY), Chris Lalancette, Julien Enoch, Mahmoud Mazouz, Tim Clephas, Yadunund, Yuyuan Yuan

0.1.0 (2025-01-02)
------------------
* An alternative middleware for ROS 2 based on Zenoh.
* Contributors: Alejandro Hernández Cordero, Alex Day, Bernd Pfrommer, ChenYing Kuo (CY), Chris Lalancette, Christophe Bedard, CihatAltiparmak, Esteve Fernandez, Franco Cipollone, Geoffrey Biggs, Hans-Martin, James Mount, Julien Enoch, Morgan Quigley, Nate Koenig, Shivang Vijay, Yadunund, Yuyuan Yuan, methylDragon
