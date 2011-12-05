#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "openflow/openflow.h"
#include "context.h"


#include "usage.h"
#include "utils.h"
#include "module_run.h"


struct option oflops_options[] = {
// 	name	, has_arg,  *var, val
	{"control-dev", required_argument, NULL, 'c'}, 		// --control-dev eth0
#if 0
	{"snmp-dev", optional_argument, NULL, 's'}, 		// --snmp-dev necsw.openflow.org
#endif
	{"data-dev", required_argument, NULL, 'd'}, 		// --data-dev eth1
	{"port", required_argument, NULL, 'p'}, 		// --port 6633
	{ 0 , 0 , 0, 0}
};

char * option_args[] =  {
	"",			// no argument
	"<required_arg>",	//required arg
	"[optional arg]",	// optional arg
};

static char * make_short_from_long(struct option long_options[]);
static void parse_test_module(oflops_context * ctx, int argc, char * argv[]);

/*****************************************************************************
 * int parse_args(oflops_context * ctx, int argc, char * argv[])
 * 	
 **/

int parse_args(oflops_context * ctx, int argc, char * argv[])
{
	int c;
	int snmp_arg = 0;
	int options_index;
	char* snmp_client;
	char* snmp_community;
	char * short_options = make_short_from_long(oflops_options);

	while(1)
	{
		c = getopt_long(argc, argv, short_options, oflops_options, &options_index);
		if( c == -1 )
			break;	// done args parsing
		switch(c)
		{
			case 'c':
				assert(OFLOPS_CONTROL == 0);
				assert(ctx->n_channels > 0);
				if(!optarg)
					usage(argv[optind], "requires argument");
				channel_info_init(&ctx->channels[OFLOPS_CONTROL],optarg);
				fprintf(stderr,"Setting control channel to %s\n", optarg);
				break;
#if 0
			case 's':
				snmp_arg++;
				assert(snmp_arg < 2);
				if(!optarg)
					usage(argv[optind], "requires argument");
				snmp_client = strtok(optarg, SNMP_DELIMITER);
				snmp_community = strtok(NULL, SNMP_DELIMITER);
				snmp_channel_init(ctx->snmp_channel_info, snmp_client, snmp_community);
				fprintf(stderr,"Adding SNMP channel on %s with community string %s.\n", 
					snmp_client, snmp_community);
				break;
#endif
			case 'd':
				if(ctx->n_channels >= ctx->max_channels)	// resize array if needed
				{
					ctx->max_channels *= 2;
					ctx->channels = realloc_and_check(ctx->channels, ctx->max_channels * sizeof(channel_info));
				}
				if(!optarg)
					usage(argv[optind], "requires argument");
				channel_info_init(&ctx->channels[ctx->n_channels++], optarg);
				fprintf(stderr,"Adding a data channel on %s\n", optarg);
				break;
			case 'p':
				ctx->listen_port = atoi(optarg);
				fprintf(stderr,"Setting Control listen port to %d\n", ctx->listen_port);
				break;
			default:
				usage("unknown option", argv[optind]);
		}
	}
	// skip ahead to any other args
	argc-=optind;
	argv+=optind;
	if(argc > 0)
		parse_test_module(ctx, argc, argv);
	return 0;
}

/****************************************************************************
 * static char * make_short_from_long(struct option long_options[]);
 **/
static char * make_short_from_long(struct option long_options[])
{
	static char buf[BUFLEN];
	int buf_index=0;
	int opt_index=0;

	bzero(buf,BUFLEN);
	while(oflops_options[opt_index].name != NULL)
	{
		buf[buf_index++] = oflops_options[opt_index].val;
		if(oflops_options[opt_index].has_arg)
			buf[buf_index++] = ':';
		opt_index++;
	}
	return buf;
}

/***************************************************************
 * void usage(char * s1, char *s2);
 * 	print usage information and exit
 **/
void usage(char * s1, char *s2)
{
	struct option * o;
	int i = 0;
	if(s1)
		fprintf(stderr, "%s",s1);
	if(s2)
		fprintf(stderr, " %s",s2);
	if (s1|| s2)
		fprintf(stderr, "\n");
	fprintf( stderr, "Usage:\noflops [options]\n");
	o = &oflops_options[i];
	do {
		fprintf(stderr, "\t-%c|--%s\t%s\n",
				o->val,
				o->name,
				option_args[o->has_arg]
				);

		i++;
		o = &oflops_options[i];
	} while(o->name);

	fprintf(stderr, "\n\nExample invocation:\n"
			"oflops -c eth0 -s 10.2.3.4:public -d eth1 -d eth2 -p 6633 liboflops_debug.so test_args\n");

	exit(1);
}
/**************************************************************************
 * static void parse_test_module(oflops_context * ctx, int argc, char * argv[]);
 * 	parse a test module from argc/argv and try loading it
 **/
static void parse_test_module(oflops_context * ctx, int argc, char * argv[])
{
	char buf[BUFLEN];
	int count=0;
	int i;

	if(argc==0)
		usage("need to specify a test_module to load\n",NULL);
	// turn all of the args into a single string
	for(i=1;((count < BUFLEN) && (i<argc)); i++)
		count += snprintf(buf,BUFLEN-count-1, " %s", argv[i]);
	if(load_test_module(ctx,argv[0],buf))
		fprintf(stderr, "Failed to load test_module %s\n", argv[0]);
}
