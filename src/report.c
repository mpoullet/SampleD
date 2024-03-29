/*
 * Example of a report-writing program that searches the ASL
 * data store for messages.
 *
 * This program used the ASL search API to find messages logged by the
 * "SampleD".  The user may specify a maximum log priority
 * and a time interval as command line options.
 * 
 * usage: report [-l max_level] [-s start_offset] [-e end offset]
 * max level must be an integer in the range 0 to 7.
 * start_offset and end_offset are integers that denote a number
 * of hours before the time at which the program is run.  
 *
 * For example, to get a report of all log messages of level 4 (Warning) 
 * or less, in the interval beginning 12 hours ago and ending 6 hours ago:
 *
 * report -l 4 -s 12 -e 6
 *
 * The default for max_level is 5 (Notice).
 * The default start time is 1 hours ago.
 * The default end time is the current time.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <asl.h>
#include <unistd.h>

#define PRINT_FMT "%-25s %s\n"

#define DEFAULT_MAX_LEVEL 5
#define DEFAULT_START_TIME_OFFSET 1
#define DEFAULT_END_TIME_OFFSET 0

void
usage()
{
	fprintf(stderr, "usage: report [-l max_level] [-s start_offset] [-e end offset]\n");
	fprintf(stderr, "max level must be an integer in the range 0 to 7.\n");
	fprintf(stderr, "Default is 5.\n");
	fprintf(stderr, "start_offset and end_offset are integers that denote a number\n");
	fprintf(stderr, "of hours before the time at which the program is run.\n");
	fprintf(stderr, "Default start_offset is 1 and end_offset is 0 (i.e. messages in the last hour).\n");
}		

int
main(int argc, char *argv[])
{
	int i, max_level, start_offset, end_offset;
	aslmsg query, msg;
	aslresponse resp;
	char tbuf[11];
        int retval = EXIT_SUCCESS;

	/*
	 * Initialize defaults.
	 */
	max_level = DEFAULT_MAX_LEVEL;
	start_offset = DEFAULT_START_TIME_OFFSET;
	end_offset = DEFAULT_END_TIME_OFFSET;

	/*
	 * Parse command line.
	 */
	while ((i = getopt(argc, argv, "l:s:e:")) != -1)
	{
		switch(i)
		{
			case 'l':
				max_level = atoi(optarg);
				if ((max_level < ASL_LEVEL_EMERG) || (max_level > ASL_LEVEL_DEBUG))
				{
					fprintf(stderr, "Invalid value for max_level.\n");
					usage();
					return EXIT_FAILURE;
				}

				break;
			case 's':
				start_offset = atoi(optarg);
				if (start_offset < 0)
				{
					fprintf(stderr, "Invalid value for start_offset.\n");
					usage();
					return EXIT_FAILURE;
				}
					
				break;
			case 'e':
				end_offset = atoi(optarg);
				if (end_offset < 0)
				{
					fprintf(stderr, "Invalid value for end_offset.\n");
					usage();
					return EXIT_FAILURE;
				}
						
				break;
		}
	}

	/* 
	 * Check that the time interval is at least an hour.
	 */
	if (start_offset <= end_offset)
	{
		fprintf(stderr, "Invalid time interval.\n");
		usage();
		return EXIT_FAILURE;
	}

	/*
	 * Create a query message.
	 */
	
	query = asl_new(ASL_TYPE_QUERY);
        if (query == NULL)
        {
            fprintf(stderr, "asl_new: unable to allocate query.\n");
            retval = EXIT_FAILURE;
            goto done;
        }
	
	/* 
         * Set the query's sender to this program.
         * asl_set_query returns 0 on success and -1 on failure
         */
        if ((asl_set_query(query, ASL_KEY_SENDER, "SampleD", ASL_QUERY_OP_EQUAL)) != 0)
        {
            fprintf(stderr, "asl_set_query: unable to set query.\n");
            retval = EXIT_FAILURE;
            goto done;
        }

	/* Level <= max_level */
	snprintf(tbuf, 10, "%d", max_level);
	if ((asl_set_query(query, ASL_KEY_LEVEL, tbuf, ASL_QUERY_OP_LESS_EQUAL | ASL_QUERY_OP_NUMERIC)) != 0)
        {
            fprintf(stderr, "asl_set_query: unable to set query.\n");
            retval = EXIT_FAILURE;
            goto done;
        }


	/* "Remote Host" has a value. */
	if ((asl_set_query(query, "Remote Host", NULL, ASL_QUERY_OP_NOT_EQUAL)) != 0)
        {
            fprintf(stderr, "asl_set_query: unable to set query.\n");
            retval = EXIT_FAILURE;
            goto done;
        }


	/*
	 * Start time is minus start_offset hours.
	 * minus sign means before the current time, "H" means the offset is in hours.
	 */
	snprintf(tbuf, 10, "-%dH", start_offset);
	if ((asl_set_query(query, ASL_KEY_TIME, tbuf, ASL_QUERY_OP_GREATER_EQUAL)) != 0)
        {
            fprintf(stderr, "asl_set_query: unable to set query.\n");
            retval = EXIT_FAILURE;
            goto done;
        }

	if (end_offset > 0)
	{
		/* End time is minus end_offset hours. */
		snprintf(tbuf, 10, "-%dH", end_offset);
		if ((asl_set_query(query, ASL_KEY_TIME, tbuf, ASL_QUERY_OP_LESS_EQUAL)) != 0)
                {
                    fprintf(stderr, "asl_set_query: unable to set query.\n");
                    retval = EXIT_FAILURE;
                    goto done;
                }
	}

	/*
	 * Perform the search.
         * asl_search will return 0 if the supplied query is NULL.
         * Otherwise it returns a valid aslresponse, even if there are no matches.
	 */
	resp = asl_search(NULL, query);
        if (resp == NULL)
        {
            fprintf(stderr, "asl_search: NULL query.\n");
            retval = EXIT_FAILURE;
            goto done;
        }
        

	/*
	 * Get the first matching message.
	 */
	msg = aslresponse_next(resp);

	/*
	 * Only print out a report if we have had any connections.
	 */
	if (msg != NULL)
	{
		printf("Connection Summary\n");
		printf("==================\n\n");
		printf(PRINT_FMT, "Time", "IP Address");
		do
		{
			const char *tim, *host;

			tim = asl_get(msg, ASL_KEY_TIME);
			host = asl_get(msg, "Remote Host");

			printf(PRINT_FMT, tim, host);
		} while ((msg = aslresponse_next(resp)) != NULL);
	}

	aslresponse_free(resp);

done:
	asl_free(query);
	return retval;
}

