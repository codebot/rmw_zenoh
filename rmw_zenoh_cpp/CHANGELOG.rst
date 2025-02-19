^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_zenoh_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.2.2 (2025-02-19)
------------------
* Introduce the advanced publisher and subscriber (`#469 <https://github.com/ros2/rmw_zenoh/issues/469>`_)
* Add tracing instrumentation using tracetools  (`#471 <https://github.com/ros2/rmw_zenoh/issues/471>`_)
* Switch to debug log if topic_name not in topic_map (`#464 <https://github.com/ros2/rmw_zenoh/issues/464>`_)
* Bump Zenoh to commit id 3bbf6af (1.2.1 + few commits) (`#461 <https://github.com/ros2/rmw_zenoh/issues/461>`_)
* Contributors: Alejandro Hernandez Cordero, Christophe Bedard, Julien Enoch, Yadunund, Yuyuan Yuan

0.2.1 (2025-02-04)
------------------
* Bump Zenoh to commit id e4ea6f0 (1.2.0 + few commits) (`#448 <https://github.com/ros2/rmw_zenoh/issues/448>`_)
* Inform users that peers will not discover and communicate with one another until the router is started (`#444 <https://github.com/ros2/rmw_zenoh/issues/444>`_)
* Clear the error after rmw_serialized_message_resize() (`#436 <https://github.com/ros2/rmw_zenoh/issues/436>`_)
* Fix `ZENOH_ROUTER_CHECK_ATTEMPTS` which was not respected (`#428 <https://github.com/ros2/rmw_zenoh/issues/428>`_)
* fix: use the default destructor to drop the member `Payload` (`#420 <https://github.com/ros2/rmw_zenoh/issues/420>`_)
* Remove `gid_hash\_` from `AttachmentData` (`#417 <https://github.com/ros2/rmw_zenoh/issues/417>`_)
* Sync the config with the default config in Zenoh. (`#413 <https://github.com/ros2/rmw_zenoh/issues/413>`_)
* fix: check the context validity before accessing the session (`#406 <https://github.com/ros2/rmw_zenoh/issues/406>`_)
* Fix wan't typo (`#401 <https://github.com/ros2/rmw_zenoh/issues/401>`_)
* Contributors: ChenYing Kuo (CY), Chris Lalancette, Julien Enoch, Mahmoud Mazouz, Tim Clephas, Yadunund, Yuyuan Yuan

0.2.0 (2025-01-02)
------------------
* An alternative middleware for ROS 2 based on Zenoh.
* Contributors: Alejandro Hernández Cordero, Alex Day, Bernd Pfrommer, ChenYing Kuo (CY), Chris Lalancette, Christophe Bedard, CihatAltiparmak, Esteve Fernandez, Franco Cipollone, Geoffrey Biggs, Hans-Martin, James Mount, Julien Enoch, Morgan Quigley, Nate Koenig, Shivang Vijay, Yadunund, Yuyuan Yuan, methylDragon
