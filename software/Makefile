all:
			cd pws_manager; make

clean:
			cd pws_manager; sudo make clean
			sudo rm -rf /pws
			rm -f /home/pi/.config/autostart/windowpy.desktop
			sudo systemctl disable pws-manager
			sudo systemctl disable pws-uploader
			sudo systemctl disable pws-webserver

install:
			cd pws_manager; sudo make install
			if [ ! -d /pws/pd ]; then \
				sudo mkdir -p /pws/pd; \
			fi
			if [ ! -d /pws/py ]; then \
				sudo mkdir -p /pws/py; \
			fi
			if [ ! -d /pws/log ]; then \
				sudo mkdir -p /pws/log; \
			fi
			sudo cp -r pd /pws
			sudo cp -r py /pws
			if [ ! -d /home/pi/.config/autostart ]; then \
				mkdir -p /home/pi/.config/autostart; \
			fi
			cp -f windowpy.desktop /home/pi/.config/autostart/
			sudo cp -f pws-manager.service /etc/systemd/system/
			sudo cp -f pws-uploader.service /etc/systemd/system/
			sudo cp -f pws-webserver.service /etc/systemd/system/
			sudo systemctl enable pws-manager
			sudo systemctl enable pws-uploader
			sudo systemctl enable pws-webserver
