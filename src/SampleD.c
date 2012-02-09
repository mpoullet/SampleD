#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <asl.h>
#include <libgen.h>
#include <launch.h>

int main(void)
{
	struct sockaddr_storage ss;
	socklen_t slen = sizeof(ss);
	struct kevent kev_init;
	struct kevent kev_listener;
	launch_data_t sockets_dict, checkin_response;
        launch_data_t checkin_request;
        launch_data_t listening_fd_array;
	size_t i;
	int kq;
	aslclient asl = NULL;
	aslmsg log_msg = NULL;
	int retval = EXIT_SUCCESS;

	/*
	 * Create a new ASL log
	 *
	 */	 
	asl = asl_open("SampleD", "Daemon", ASL_OPT_STDERR);
	log_msg = asl_new(ASL_TYPE_MSG);
	asl_set(log_msg, ASL_KEY_SENDER, "SampleD");
	
	/*
	 * Create a new kernel event queue
	 * that we'll use for our notification.
	 *
         * Note the use of the '%m' formatting character.
         * ASL will replace %m with the error string associated 
         * with the current value of errno.
         */
	if (-1 == (kq = kqueue())) {
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "kqueue(): %m");
		retval = EXIT_FAILURE;
		goto done;
	}

	/*
	 * Register ourselves with launchd.
	 * 
	 */
        if ((checkin_request = launch_data_new_string(LAUNCH_KEY_CHECKIN)) == NULL) {
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "launch_data_new_string(\"" LAUNCH_KEY_CHECKIN "\") Unable to create string.");
		retval = EXIT_FAILURE;
		goto done;
	}

	if ((checkin_response = launch_msg(checkin_request)) == NULL) {
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "launch_msg(\"" LAUNCH_KEY_CHECKIN "\") IPC failure: %m");
		retval = EXIT_FAILURE;
		goto done;
	}

	if (LAUNCH_DATA_ERRNO == launch_data_get_type(checkin_response)) {
		errno = launch_data_get_errno(checkin_response);
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "Check-in failed: %m");
		retval = EXIT_FAILURE;
		goto done;
	}

        launch_data_t the_label = launch_data_dict_lookup(checkin_response, LAUNCH_JOBKEY_LABEL);
	if (NULL == the_label) {
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "No label found");
		retval = EXIT_FAILURE;
		goto done;
	}
        asl_log(asl, log_msg, ASL_LEVEL_NOTICE, "Label: %s", launch_data_get_string(the_label));

	
	/*
	 * Retrieve the dictionary of Socket entries in the config file
	 */
	sockets_dict = launch_data_dict_lookup(checkin_response, LAUNCH_JOBKEY_SOCKETS);
	if (NULL == sockets_dict) {
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "No sockets found to answer requests on!");
		retval = EXIT_FAILURE;
		goto done;
	}

	if (launch_data_dict_get_count(sockets_dict) > 1) {
		asl_log(asl, log_msg, ASL_LEVEL_WARNING, "Some sockets will be ignored!");
	}

	/*
	 * Get the dictionary value from the key "MyListenerSocket", as defined in the com.apple.dts.SampleD.plist file.
	 */
	listening_fd_array = launch_data_dict_lookup(sockets_dict, "MyListenerSocket");
	if (NULL == listening_fd_array) {
		asl_log(asl, log_msg, ASL_LEVEL_ERR, "No known sockets found to answer requests on!");
		retval = EXIT_FAILURE;
		goto done;
	}

	/*
	 * Initialize a new kernel event.  This will trigger when
	 * a connection occurs on our listener socket.
	 *
	 */
	for (i = 0; i < launch_data_array_get_count(listening_fd_array); i++) {
		launch_data_t this_listening_fd = launch_data_array_get_index(listening_fd_array, i);

		EV_SET(&kev_init, launch_data_get_fd(this_listening_fd), EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent(kq, &kev_init, 1, NULL, 0, NULL) == -1) {
			asl_log(asl, log_msg, ASL_LEVEL_DEBUG, "kevent(): %m");
			retval = EXIT_FAILURE;
			goto done;
		}
	}

	launch_data_free(checkin_response);

	for (;;) {
		FILE *the_stream;
		int filedesc, gai_r;
		char nodename[1024];
		char servname[1024];

		/*
		 *
		 * Get the next event from the kernel event queue.
		 *
		 */
		if ((filedesc = kevent(kq, NULL, 0, &kev_listener, 1, NULL)) == -1) {
			asl_log(asl, log_msg, ASL_LEVEL_ERR, "kevent(): %m");
			retval = EXIT_FAILURE;
			goto done;
		} else if (filedesc == 0) {
			retval = EXIT_SUCCESS;
			goto done;
		}

		/*
		 *
		 * Accept an incoming connection.
		 *
		 */
		if ((filedesc = accept(kev_listener.ident, (struct sockaddr *)&ss, &slen)) == -1) {
			asl_log(asl, log_msg, ASL_LEVEL_ERR, "accept(): %m");
			continue; /* this isn't fatal */
		}

		/*
		 *
		 * Extract the client's host and port names so we 
		 * can print them out.
		 *
		 */
		gai_r = getnameinfo((struct sockaddr *)&ss, slen, nodename, sizeof(nodename),
				servname, sizeof(servname), NI_NUMERICSERV | NI_NUMERICHOST);

		if (0 != gai_r) {
			/* Error occured - log it */
			asl_log(asl, log_msg, ASL_LEVEL_NOTICE, "getnameinfo(): %s", gai_strerror(gai_r));
		} else {
			/*
			 * Successfully retrieved.  
			 * Set our unique ASL keys/value pairs. 
			 */
			asl_set(log_msg, "Remote Host", nodename);
			asl_set(log_msg, "Remote Port", servname);
						
			/*
			 * Send our log message off to the syslogd server.  
			 */
			asl_log(asl, log_msg, ASL_LEVEL_NOTICE, "Connection established");

			/*
			 * Clean out the log message...
			 */
			asl_unset(log_msg, "Remote Host");
			asl_unset(log_msg, "Remote Port");
		}

		/*
		 * Open the stream and message the client. 
		 */
		the_stream = fdopen(filedesc, "r+");
		asl_log(asl, log_msg, ASL_LEVEL_NOTICE, "got file descriptor %d", filedesc);

		if (the_stream) {
			fprintf(the_stream, "hello world!\n");
			fclose(the_stream);
		} else {
			close(filedesc);
		}

		sleep(1);
	}
done:
	asl_close(asl);
	return retval;
}

