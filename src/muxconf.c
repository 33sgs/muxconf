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

const char delim[2] = " ";

// This function provides the decoding for setting the switches on the MAX14661
// It returns the bit to set
int decodeMAX14661(int inputPin, int commonPin){
	return (16-inputPin + 16 * (2-commonPin));
}

void writeConfigBinary(int **config, FILE * outputFile){
	int i, j;
	for(i = 0; i < NUM_DEVICES; i++){
		for(j = DEVICE_DATA_LENGTH-1; j >= 0; j--){
			fputc('0' + config[i][j], outputFile);
		}
		fputs("\n", outputFile);

	}
}

void writeConfigDecimal(int **config, FILE * outputFile){
	int i, j;
	long long counter;
	for(i = 0; i < NUM_DEVICES; i++){
		counter = 0;
		for(j = 0; j < DEVICE_DATA_LENGTH; j++){
			counter += (1 << (j)) * config[i][j];
		}
		fprintf(outputFile, "%lld\n", counter);
	}
}


struct deviceMapping initDeviceMapping(int maxDevices){
	struct deviceMapping newDeviceMapping;
	newDeviceMapping.deviceMapList = malloc(sizeof(struct deviceMap) * maxDevices);
	newDeviceMapping.maxDevices = maxDevices;
	return newDeviceMapping;
}

struct nodeMapping initNodeMapping(int maxNodes){
	struct nodeMapping newNodeMapping;
	newNodeMapping.nodeToRow = malloc(sizeof(int) * maxNodes);
	newNodeMapping.nodeCommon = malloc(sizeof(int) * maxNodes);
	newNodeMapping.maxNodes = maxNodes;
	return newNodeMapping;
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
			token = strtok(mapline, delim);
			if(!strcmp(token, "matrix_rows")) {
				token = strtok(NULL, delim);
				(*outMapdata).matrix_rows = atoi(token);
			} else if(!strcmp(token, "matrix_cols")) {
				token = strtok(NULL, delim);
				(*outMapdata).matrix_cols = atoi(token);
			} else if(!strcmp(token, "mux_count")) {
				token = strtok(NULL, delim);
				(*outMapdata).mux_count = atoi(token);
			} else if(!strcmp(token, "device_count")) {
				token = strtok(NULL, delim);
				(*outMapdata).device_count = atoi(token);
			} else if(!strcmp(token, "nodes_per_row")) {
				token = strtok(NULL, delim);
				(*outMapdata).nodes_per_row = atoi(token);
			} else if(!strcmp(token, "inputs_per_col")) {
				token = strtok(NULL, delim);
				(*outMapdata).inputs_per_col = atoi(token);
			} else {
				fprintf(stderr, "mapdata section of mapfile contains an invalid property: %s\n", token);
			}
		}
	}
	return foundEnd;
}

struct inputMapping initInputMapping(int maxInputs){
	struct inputMapping newInputMapping;
	newInputMapping.maxInputs = maxInputs;
	newInputMapping.inputToCol = malloc(sizeof(int) * maxInputs);
	newInputMapping.inputPin = malloc(sizeof(int) * maxInputs);
	return newInputMapping;
}


int readInputMapping(struct inputMapping *outInputMapping, FILE * mapfile){
	char * token;
	int inputNum, colNum, pinNum;
	char mapline[MAX_NETLIST_LINE];
	while(fgets(mapline, MAX_NETLIST_LINE, mapfile) != NULL){
		if(!strcmp(mapline, "END inputmapping\n")) {
			break;
		} else {

			token = strtok(mapline, delim);
			inputNum = atoi(token);

			token = strtok(NULL, delim);
			colNum = atoi(token);

			token = strtok(NULL, delim);
			pinNum = atoi(token);

			if((inputNum < outInputMapping->maxInputs) && (inputNum >= 0)){
#ifdef VERBOSE
				fprintf(stderr, "Input %d is in column %d on pin %d\n", inputNum, colNum, pinNum);
#endif
				outInputMapping->inputToCol[inputNum] = colNum;
				outInputMapping->inputPin[inputNum] = pinNum;
			}
		}
	}
	return 0;
}

int readNodeMapping(struct nodeMapping *outNodeMapping, FILE * mapfile){
	char * token;
	int nodeNum, rowNum, comNum;
	char mapline[MAX_NETLIST_LINE];
	while(fgets(mapline, MAX_NETLIST_LINE, mapfile) != NULL){
		if(!strcmp(mapline, "END nodemapping\n")) {
			break;
		} else {

			token = strtok(mapline, delim);
			nodeNum = atoi(token);

			token = strtok(NULL, delim);
			rowNum = atoi(token);

			token = strtok(NULL, delim);
			comNum = atoi(token);


			if((nodeNum < outNodeMapping->maxNodes) && (nodeNum >= 0)){
#ifdef VERBOSE
				fprintf(stderr, "Node %d is in column %d on common %d\n", nodeNum, rowNum, comNum);
#endif
				outNodeMapping->nodeToRow[nodeNum] = rowNum;
				outNodeMapping->nodeCommon[nodeNum] = comNum;
			}
		}
	}
	return 0;
}




int readDeviceMapping(struct deviceMapping *outDeviceMapping, FILE * mapfile){
	char * token;
	int count = 0;
	int portCounter;
	char mapline[MAX_NETLIST_LINE];
	while(count < outDeviceMapping->maxDevices){
		fgets(mapline, MAX_NETLIST_LINE, mapfile);
		if(mapline == NULL) {
			break;
		} else if(!strcmp(mapline, "END devicemapping")) {
			break;
		} else {
			token = strtok(mapline, delim);
			strcpy(outDeviceMapping->deviceMapList[count].name, token);

			token = strtok(NULL, delim);
			outDeviceMapping->deviceMapList[count].ports = atoi(token);
			portCounter = 0;
			while(portCounter < outDeviceMapping->deviceMapList[count].ports) {
				token = strtok(NULL, delim);
				outDeviceMapping->deviceMapList[count].portInput[portCounter++] = atoi(token);
			}
			count++;
		}


	}
	outDeviceMapping->numDevices = count;
	return 0;
}


struct muxMapping initMuxMapping(int numRows, int numCols){
	int i;
	struct muxMapping newMuxMapping;
	newMuxMapping.numRows = numRows;
	newMuxMapping.numCols = numCols;

	newMuxMapping.muxMap = (int **) malloc(sizeof(int *) * numCols);
	for(i = 0; i < numCols; i++){
		newMuxMapping.muxMap[i] = (int *) malloc(sizeof(int) * numRows);
	}
	return newMuxMapping;
}

int readMuxMapping(struct muxMapping *outMuxMapping, FILE * mapfile){
	char * token;
	int row = 0;
	int col;
	char mapline[MAX_NETLIST_LINE];

	while(fgets(mapline, MAX_NETLIST_LINE, mapfile) != NULL){
		if((!strcmp(mapline, "END muxmapping")) || row >= outMuxMapping->numRows) {
			break;
		} else {
			col = 0;
			token = strtok(mapline, delim);
			while((col < outMuxMapping->numCols) && (token != NULL)){
				outMuxMapping->muxMap[col++][row] = atoi(token);
				token = strtok(NULL, delim);
			}
			row++;
		}


	}
	return 0;
}

int ** initConfigBytes(int muxCount, int numBytes){
	int ** newConfigBytes;
	newConfigBytes = (int **) malloc(sizeof(int *) * muxCount);
	for(int i = 0; i < muxCount; i++){
		newConfigBytes[i] = (int *) calloc(numBytes, sizeof(int));
	}
	return newConfigBytes;
}

int main(int argc, char** argv){
	FILE * outputFile;
	FILE * mapfile;
	FILE * netlist;

	char line[MAX_NETLIST_LINE];
	char * token;

	struct mapdata mainMapData;
	struct deviceMapping currDeviceMapping;
	struct nodeMapping currNodeMapping;
	struct inputMapping currInputMapping;
	struct muxMapping currMuxMapping;

	int ret;

	int ** configBytes;
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
	currDeviceMapping = initDeviceMapping(mainMapData.device_count);

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
			ret = readDeviceMapping(&currDeviceMapping, mapfile);
			if(ret == -1){
				fprintf(stderr, "Failed to read deviceMapping\n");
				exit(1);
			}
			break;
		}
	}

	//////
	// Get node mapping and store it as array
	//////

	currNodeMapping = initNodeMapping(mainMapData.nodes_per_row * mainMapData.matrix_rows);

#ifdef VERBOSE
	fprintf(stderr, "Max nodes: %d\n", currNodeMapping.maxNodes);
#endif

	// Seek start of file
	ret = fseek(mapfile, 0, SEEK_SET);
	if(ret){
		perror("Failed to seek mapfile");
		exit(1);
	}
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		else if(!strcmp(line, "BEGIN nodemapping\n")) {
			ret = readNodeMapping(&currNodeMapping, mapfile);
			if(ret == -1){
				fprintf(stderr, "Failed to read nodeMapping\n");
				exit(1);
			}
			break;
		}
	}

	//////
	// Get input mapping and store it as array
	// Map inputs to columns
	//////

	// Initialize the inputMapping struct
	currInputMapping = initInputMapping(mainMapData.inputs_per_col * mainMapData.matrix_cols);

#ifdef VERBOSE
	fprintf(stderr, "Max inputs: %d\n", currInputMapping.maxInputs);
#endif

	// Seek start of file
	ret = fseek(mapfile, 0, SEEK_SET);
	if(ret){
		perror("Failed to seek mapfile");
		exit(1);
	}
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		else if(!strcmp(line, "BEGIN inputmapping\n")) {
			ret = readInputMapping(&currInputMapping, mapfile);
			if(ret == -1){
				fprintf(stderr, "Failed to read inputmapping\n");
				exit(1);
			}
			break;
		}
	}

	//////
	// Get mux mapping and store as array
	//////
	currMuxMapping = initMuxMapping(mainMapData.matrix_rows, mainMapData.matrix_cols);
#ifdef VERBOSE
	fprintf(stderr, "\nMatrix size: %dx%d\n", currMuxMapping.numCols, currMuxMapping.numRows);
#endif
	// Seek start of file
	ret = fseek(mapfile, 0, SEEK_SET);
	if(ret){
		perror("Failed to seek mapfile");
		exit(1);
	}
	while(fgets(line, MAX_NETLIST_LINE, mapfile) != NULL){
		if(line[0] == '.' || line[0] == '*')
			continue;
		else if(!strcmp(line, "BEGIN muxmapping\n")) {
			ret = readMuxMapping(&currMuxMapping, mapfile);
			if(ret == -1){
				fprintf(stderr, "Failed to read muxmapping\n");
				exit(1);
			}
			break;
		}
	}

#ifdef VERBOSE
	fprintf(stderr, "\n");
	for(int q = 0; q < mainMapData.matrix_cols; q++){
		for(int k = 0; k < mainMapData.matrix_rows; k++){
			fprintf(stderr, "Device %d: column %d, row %d\n", currMuxMapping.muxMap[q][k], q, k);
		}

	}
#endif


	//////
	// Read netlist into memory
	//////

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

		token = strtok(line, delim);
		strcpy(fileData[lineCounter][lineToken], token);
		lineToken++;
		if(lineToken >= MAX_LINE_TOKENS){
			fprintf(stderr, "Error: line too long\n");
			exit(1);
		}

		while( (token = strtok(NULL, delim)) != NULL){
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
	for(int i = 0; i < lineCounter; i++){
		for(int j = 0; j < lineLength[i]; j++){
			fprintf(stderr, "(%d, %d): %s\n", i, j, fileData[i][j]);
		}
	}
#endif
	//////
	// Process netlist and make connections in config bits
	//////


	configBytes = initConfigBytes(mainMapData.mux_count, DEVICE_DATA_LENGTH);

	int currNode, currInput, currCol, currRow, currMux, byteToSet;
	// Go through each netlist line
	for(int i = 0; i < lineCounter; i++){
		// Find the matching part
		for(int q = 0; q < currDeviceMapping.numDevices; q++){
			// Find a match
			if(!strcmp(currDeviceMapping.deviceMapList[q].name, fileData[i][0])){
				// Go through device ports
				for(int j = 0; j < currDeviceMapping.deviceMapList[q].ports; j++){
					currNode = atoi(fileData[i][j+1]);
					currInput = currDeviceMapping.deviceMapList[q].portInput[j];
#ifdef VERBOSE
					printf("Connect input %d to node %d\n", currInput, currNode);
#endif
					// Get the matrix column
					currCol = currInputMapping.inputToCol[currInput];
					// Get the matrix row
					currRow = currNodeMapping.nodeToRow[currNode];
					// Find corresponding mux
					currMux = currMuxMapping.muxMap[currCol][currRow];

#ifdef VERBOSE
					printf("\tCol %d Row %d (Mux %d)\n", currCol, currRow, currMux);
#endif
					// Find the byte for the given switch
					byteToSet = currInputMapping.inputPin[currInput] + 16 * currNodeMapping.nodeCommon[currNode];
					configBytes[currMux - 1][byteToSet] = 1;
				}

			}

		}
	}

	//////
	// Write out config bits
	//////
#ifdef VERBOSE
	fprintf(stderr, "\nConfig Bits:\n");
	writeConfigBinary(configBytes, stdout);
#endif
	writeConfigDecimal(configBytes, outputFile);
	//////
	// Free memory
	//////

	return 0;
}

