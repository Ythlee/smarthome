all:
	-make -C client/linux/
	-cp client/linux/src/linux_client output/client/linux/

	-make -C device/
	-cp device/airConditioning/air_conditioning output/device/airConditioning

	-cp device/refrigerator/src/refrigerator output/device/refrigerator
	-cp device/refrigerator/src/test.264 output/device/refrigerator

	-make -C server/
	-cp server/server_alarm/server_alarm output/server/server_alarm
	-cp server/server_device/server_device output/server/server_device
	-cp server/server_media/server_media output/server/server_media
	-cp server/server_user/server_user output/server/server_user
	-cp server/server_master/master output/server/server_master
	-cp server/server_master/server_close.sh output/server/server_master
	-cp server/server_master/server_monitor.sh output/server/server_master

clean:
	-make clean -C client/linux/
	-rm output/client/linux/linux_client

	-make clean -C device/
	-rm output/device/airConditioning/air_conditioning
	-rm output/device/refrigerator/refrigerator

	-make clean -C server/
	-rm output/server/server_alarm/server_alarm
	-rm output/server/server_device/server_device
	-rm output/server/server_media/server_media
	-rm output/server/server_user/server_user
	-rm output/server/server_master/master
	-rm output/server/server_master/monitor_log.txt