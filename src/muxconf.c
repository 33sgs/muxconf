#include "muxconf.h"

#define MAX_NETLIST_LINE 1024
#define MAX_LINE_TOKENS 16
#define MAX_TOKEN_SIZE 16
#define MAX_LINES 16

#define MATRIX_WIDTH 16 // Nodes
#define MATRIX_HEIGHT 16 // Part pins

#define PART_LENGTH 50
#define MAX_PORTS 28
#define MAX_PARTS 16
#define NUM_DEVICES 8
#define DEVICE_DATA_LENGTH 32
#define MAX_NODES 16

const char mapfileDelim[2] = " ";

int mapNodes(struct nodeMapping **outputList, FILE * mapfile){
	int numNodes = 0;
	struct nodeMapping *nodeList = malloc(MAX_NODES * sizeof(struct nodeMapping));
	char line[MAX_NETLIST_LINE];
	const char t[2] = " ";
	char *token;
	int nodeNum;
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		if(!strcmp(line, "END nodemapping\n"))
			break;
		// Get node number
		token = strtok(line, t);
		nodeNum = atoi(token);

		// Get the device this node is located on
		token = strtok(NULL, t);
		nodeList[nodeNum].device = atoi(token);

		// Get the port this node is located on
		token = strtok(NULL, t);
		nodeList[nodeNum].common = atoi(token);

#ifdef VERBOSE
		fprintf(stderr, "Node %d is located on device %d terminal %d\n", nodeNum, nodeList[nodeNum].device, nodeList[nodeNum].common);
#endif

		numNodes++;
	}
	*outputList = nodeList;
	return numNodes;

}
/*
int mapParts (struct partMapping **outputList, FILE * mapfile){
	int numParts = 0;
	struct partMapping *partList = malloc(MAX_PARTS * sizeof(struct partMapping));
	char line[MAX_NETLIST_LINE];
	const char t[2] = " ";
	char *token;
	int k;
	fprintf(stdout, "\n");
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		if(!strcmp(line, "END devicemapping\n"))
			break;

		token = strtok(line, t);
		fprintf(stderr, "%s: ", token);
		strcpy(partList[numParts].name, token);

		token = strtok(NULL, t);
		partList[numParts].ports = atoi(token);

		fprintf(stdout, "%d port device\n", partList[numParts].ports);

		for(k = 0; k < partList[numParts].ports; k++){
			token = strtok(NULL, t);
			partList[numParts].portDevice[k] = atoi(token);

			token = strtok(NULL, t);
			partList[numParts].portPin[k] = atoi(token);

#ifdef VERBOSE
			fprintf(stderr, "\tPort %d connected to device %d pin %d\n", k+1, partList[numParts].portDevice[k], partList[numParts].portPin[k]);
#endif

		}
		numParts++;
	}
	*outputList = partList;
	return numParts;

}
*/

// This function provides the decoding for setting the switches on the MAX14661
// It returns the bit to set
int decodeMAX14661(int inputPin, int commonPin){
	return (16-inputPin + 16 * (2-commonPin));
}

void writeConfigBinary(int (*config)[DEVICE_DATA_LENGTH], FILE * outputFile){
	int i, j;
	for(i = 0; i < NUM_DEVICES; i++){
		for(j = 0; j < DEVICE_DATA_LENGTH; j++){
			fputc('0' + config[i][j], outputFile);
		}
		fputs("\n", outputFile);

	}
}

void writeConfigDecimal(int (*config)[DEVICE_DATA_LENGTH], FILE * outputFile){
	int i, j, counter;
	for(i = 0; i < NUM_DEVICES; i++){
		counter = 0;
		for(j = 0; j < DEVICE_DATA_LENGTH; j++){
			counter += (1 << (DEVICE_DATA_LENGTH - 1 - j)) * config[i][j];
		}
		fprintf(outputFile, "%d\n", counter);
	}
}


struct deviceMapping createDeviceMapping(int maxDevices){
	struct deviceMapping newDeviceMapping;
	newDeviceMapping.deviceMapList = malloc(sizeof(struct deviceMap) * maxDevices);
	newDeviceMapping.maxDevices = maxDevices;
	return newDeviceMapping;
}

int readMapData(struct mapdata* outMapdata, FILE * mapfile){
	char * token;
	int foundEnd = -1;
	char mapline[MAX_NETLIST_LINE];
	while(fgets(mapline, MAX_NETLIST_LINE, mapfile) != NULL){
		if(mapline[0] == '.' || mapline[0] == '*') {
			continue;
		}
		else if(!strcmp(mapline, "END mapdata\n")) {
			foundEnd = 0;
			break;
		}
		else {
			token = strtok(mapline, mapfileDelim);
			if(!strcmp(token, "matrix_rows")) {
				token = strtok(NULL, mapfileDelim);
				(*outMapdata).matrix_rows = atoi(token);
			} else if(!strcmp(token, "matrix_cols")) {
				token = strtok(NULL, mapfileDelim);
				(*outMapdata).matrix_cols = atoi(token);
			} else if(!strcmp(token, "mux_count")) {
				token = strtok(NULL, mapfileDelim);
				(*outMapdata).mux_count = atoi(token);
			} else if(!strcmp(token, "device_count")) {
				token = strtok(NULL, mapfileDelim);
				(*outMapdata).device_count = atoi(token);
			} else if(!strcmp(token, "nodes_per_row")) {
				token = strtok(NULL, mapfileDelim);
				(*outMapdata).nodes_per_row = atoi(token);
			} else if(!strcmp(token, "inputs_per_col")) {
				token = strtok(NULL, mapfileDelim);
				(*outMapdata).inputs_per_col = atoi(token);
			} else {
				fprintf(stderr, "mapdata section of mapfile contains an invalid property: %s\n", token);
			}
		}
	}
	return foundEnd;
}

int readDeviceMapping(struct deviceMapping *outDeviceMapping, FILE * mapfile){
	char * token;
	int count = 0;
	int portCounter;
	char mapline[MAX_NETLIST_LINE];
	while(count < outDeviceMapping->maxDevices){
		fgets(mapline, MAX_NETLIST_LINE, mapfile);
		if(mapline == NULL) {
			printf("got null line\n");
			break;
		} else if(!strcmp(mapline, "END devicemapping")) {
			break;
		} else {
			token = strtok(mapline, mapfileDelim);
			strcpy(outDeviceMapping->deviceMapList[count].name, token);

			token = strtok(NULL, mapfileDelim);
			outDeviceMapping->deviceMapList[count].ports = atoi(token);
			portCounter = 0;
			while(portCounter < outDeviceMapping->deviceMapList[count].ports) {
				token = strtok(NULL, mapfileDelim);
				outDeviceMapping->deviceMapList[count].portInput[portCounter++] = atoi(token);
			}
			count++;
		}


	}
	outDeviceMapping->numDevices = count;
	return 0;
}

int main(int argc, char** argv){
	FILE * outputFile;
	FILE * mapfile;
	FILE * netlist;
	char line[MAX_NETLIST_LINE];
	struct mapdata mainMapData;
	struct deviceMapping mainDeviceMapping;
	int ret;
	//////
	// Open the files
	//////

	fprintf(stdout, "Netlist path: %s\n", argv[1]);
	fprintf(stdout, "Mapfile path: %s\n", argv[2]);
	fprintf(stdout, "Output path: %s\n", argv[3]);

	netlist = fopen(argv[1], "r");
	if(netlist == NULL){
		perror("Failed to open netlist");
		exit(1);
	}
	mapfile = fopen(argv[2], "r");
	if(mapfile == NULL){
		perror("Failed to open mapfile");
		exit(1);
	}

	outputFile = fopen(argv[3], "w");
	if(outputFile == NULL){
		perror("Failed to open output file");
		exit(1);
	}
	//////
	// Get mapdata from mapfile and store it
	//////

	// Seek start of file
	ret = fseek(mapfile, 0, SEEK_SET);
	if(ret){
		perror("Failed to seek mapfile");
		exit(1);
	}
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		else if(!strcmp(line, "BEGIN mapdata\n")) {
			ret = readMapData(&mainMapData, mapfile);
			if(ret == -1){
				fprintf(stderr, "Failed to read mapdata\n");
				exit(1);
			}
			break;
		}
	}

// Print mapdata information
#ifdef VERBOSE
	printf("maxtrix_rows = %d\n", mainMapData.matrix_rows);
	printf("maxtrix_cols = %d\n", mainMapData.matrix_cols);
	printf("mux_count = %d\n", mainMapData.mux_count);
	printf("device_count = %d\n", mainMapData.device_count);
	printf("nodes_per_row = %d\n", mainMapData.nodes_per_row);
	printf("inputs_per_col = %d\n", mainMapData.inputs_per_col);
#endif

	//////
	// Get devicemapping and store it as struct
	//////

	// Initialize deviceMapping
	mainDeviceMapping = createDeviceMapping(mainMapData.device_count);

	// Seek start of file
	ret = fseek(mapfile, 0, SEEK_SET);
	if(ret){
		perror("Failed to seek mapfile");
		exit(1);
	}
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		else if(!strcmp(line, "BEGIN devicemapping\n")) {
			ret = readDeviceMapping(&mainDeviceMapping, mapfile);
			if(ret == -1){
				fprintf(stderr, "Failed to read deviceMapping\n");
				exit(1);
			}
			break;
		}
	}

	return 0;
	//////
	// Get node mapping and store it as array
	//////

	//////
	// Get mux mapping and store as array
	//////

	//////
	// Read netlist into memory
	//////

	//////
	// Process netlist and make connections in config bits
	//////

	//////
	// Write out config bits
	//////

	//////
	// Free memory
	//////

	/*
	const char t[2] = " ";
	char *token;

	struct partMapping *partList;
	struct nodeMapping *nodeList;

	int config[NUM_DEVICES][DEVICE_DATA_LENGTH];
	memset(config, 0, sizeof(config));


	int numParts, numNodes, i, j, q, node;

	if(argc < 2){
		fprintf(stderr, "Usage: ./muxconf path-to-netlist path-to-mapfile path-to-output-file\n");
		exit(1);
	}


	fprintf(stdout, "Netlist path: %s\n", argv[1]);
	fprintf(stdout, "Mapfile path: %s\n", argv[2]);
	fprintf(stdout, "Output path: %s\n", argv[3]);

	netlist = fopen(argv[1], "r");
	if(netlist == NULL){
		perror("Failed to open netlist");
		exit(1);
	}
	mapfile = fopen(argv[2], "r");
	if(mapfile == NULL){
		perror("Failed to open mapfile");
		exit(1);
	}


	// Open the output file
	outputFile = fopen(argv[3], "w");
	if(outputFile == NULL){
		perror("Failed to open output file");
		exit(1);
	}


	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		else if(!strcmp(line, "BEGIN devicemapping\n"))
			numParts = mapParts(&partList, mapfile);
		else if(!strcmp(line, "BEGIN nodemapping\n"))
			numNodes = mapNodes(&nodeList, mapfile);
	}

	fprintf(stdout, "\nTotal number of parts: %d\n", numParts);
	fprintf(stdout, "Total number of nodes: %d\n", numNodes);

	// GET FILE DATA
	char fileData[MAX_LINES][MAX_LINE_TOKENS][MAX_TOKEN_SIZE];
#ifdef VERBOSE
	int lineLength[MAX_LINES];
#endif
	int lineCounter, lineToken;

	lineCounter = 0;
	while(fgets(line, MAX_NETLIST_LINE, netlist) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		lineToken = 0;

		token = strtok(line, t);
		strcpy(fileData[lineCounter][lineToken], token);
		lineToken++;
		if(lineToken >= MAX_LINE_TOKENS){
			fprintf(stderr, "Error: line too long\n");
			exit(1);
		}

		while( (token = strtok(NULL, t)) != NULL){
			strcpy(fileData[lineCounter][lineToken], token);
			lineToken++;
			if(lineToken >= MAX_LINE_TOKENS){
				fprintf(stderr, "Error: line too long\n");
				exit(1);
			}
		}
#ifdef VERBOSE
		lineLength[lineCounter] = lineToken;
#endif
		lineCounter++;
		if(lineCounter >= MAX_LINES){
			fprintf(stderr, "Error: too many lines in netlist\n");
		}
	}

#ifdef VERBOSE
	for(i = 0; i < lineCounter; i++){
		for(j = 0; j < lineLength[i]; j++){
			fprintf(stderr, "(%d, %d): %s\n", i, j, fileData[i][j]);
		}
	}
#endif

	// Map parts to nodes
	// Go through every line in netlist
	for(i = 0; i < lineCounter; i++){
		// Search for the matching part
		for(j = 0; j < numParts; j++){
			// Check if names match
			if( !strcmp(fileData[i][0], partList[j].name) ){
				// Map every port to a node
				for(q = 0; q < partList[j].ports; q++){
					node = atoi(fileData[i][q+1]);
#ifdef VERBOSE
					fprintf(stderr, "%s port %d mapped to node %d\n", partList[j].name, q+1, atoi(fileData[i][q+1]));
					fprintf(stderr, "Connect device %d input %d to node %d (device %d common %d)\n", partList[j].portDevice[q], partList[j].portPin[q], node, nodeList[node].device, nodeList[node].common);
#endif
					config[nodeList[node].device - 1][decodeMAX14661(partList[j].portPin[q], nodeList[node].common)] = 1;
				}
				break;
			}
		}

	}
	writeConfigBinary(config, outputFile);
	fprintf(stdout, "\nFinished writting config file\n");

	return 0;
	*/
}

