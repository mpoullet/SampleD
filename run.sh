#/bin/sh
#set -x
PLIST="src/com.apple.dts.SampleD.plist"
echo "Generate a Launch Daemon called SampleD in build/"
./build_mac.sh
echo "Copy the daemon in a root-owned, non-user directory: /tmp/SampleD"
sudo cp build/Debug/SampleD /tmp
echo "Load the plist (Property List) associated with the daemon"
sudo chown root ${PLIST}
sudo launchctl load ${PLIST}
echo "Now let's try to talk with the socket specified in the plist: 12345"
./src/client.pl
echo "Unload the plist"
sudo launchctl unload ${PLIST}
sudo chown mpoullet ${PLIST}
echo "syslog -d /private/var/log/asl | grep SampleD"
