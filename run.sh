#/bin/sh
set -x
echo "Generate a Launch Daemon called SampleD in build/"
make clean all
echo "Copy the daemon in a root-owned, non-user directory: /tmp/SampleD"
sudo cp build/SampleD /tmp
echo "Load the plist (Property List) associated with the daemon"
sudo launchctl load src/com.apple.dts.SampleD.plist
echo "Now let's try to talk with the socket specified in the plist: 12345"
./src/client.pl
# ./src/client.pl &
echo "Unload the plist"
sudo launchctl unload src/com.apple.dts.SampleD.plist
echo "syslog -d /private/var/log/asl"
