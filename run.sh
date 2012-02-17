#/bin/sh
PLIST="src/com.apple.dts.SampleD.plist"
LAUNCHD="build/Debug/SampleD"
./build_mac.sh
sudo cp ${LAUNCHD} /tmp
plutil -lint ${PLIST}
sudo chown root ${PLIST}
sudo launchctl load ${PLIST}
./src/client.pl
sudo launchctl unload ${PLIST}
sudo chown mpoullet ${PLIST}
echo "syslog -d /private/var/log/asl | grep SampleD"
