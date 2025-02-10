^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_zenoh_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.3.1 (2025-02-04)
------------------
* Bump Zenoh to commit id e4ea6f0 (1.2.0 + few commits) (`#446 <https://github.com/ros2/rmw_zenoh/issues/446>`_)
* Inform users that peers will not discover and communicate with one another until the router is started (`#440 <https://github.com/ros2/rmw_zenoh/issues/440>`_)
* Clear the error after rmw_serialized_message_resize() (`#435 <https://github.com/ros2/rmw_zenoh/issues/435>`_)
* Fix `ZENOH_ROUTER_CHECK_ATTEMPTS` which was not respected (`#427 <https://github.com/ros2/rmw_zenoh/issues/427>`_)
* fix: use the default destructor to drop the member `Payload` (`#419 <https://github.com/ros2/rmw_zenoh/issues/419>`_)
* Remove `gid_hash\_` from `AttachmentData` (`#416 <https://github.com/ros2/rmw_zenoh/issues/416>`_)
* Sync the config with the default config in Zenoh. (`#396 <https://github.com/ros2/rmw_zenoh/issues/396>`_)
* fix: check the context validity before accessing the session (`#403 <https://github.com/ros2/rmw_zenoh/issues/403>`_)
* Fix wan't typo (`#400 <https://github.com/ros2/rmw_zenoh/issues/400>`_)
* Contributors: ChenYing Kuo (CY), Chris Lalancette, Julien Enoch, Mahmoud Mazouz, Tim Clephas, Yuyuan Yuan, Yadunund

0.3.0 (2025-01-02)
------------------
* An alternative middleware for ROS 2 based on Zenoh.
* Contributors: Alejandro Hernández Cordero, Alex Day, Bernd Pfrommer, ChenYing Kuo (CY), Chris Lalancette, Christophe Bedard, CihatAltiparmak, Esteve Fernandez, Franco Cipollone, Geoffrey Biggs, Hans-Martin, James Mount, Julien Enoch, Morgan Quigley, Nate Koenig, Shivang Vijay, Yadunund, Yuyuan Yuan, methylDragon
