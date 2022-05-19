#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MAPFILE_LINE 512
#define MAX_NETLIST_LINE 1024
#define MAX_LINE_TOKENS 16
#define MAX_TOKEN_SIZE 16
#define MAX_LINES 16

#define MATRIX_WIDTH 16 // Nodes
#define MATRIX_HEIGHT 16 // Part pins

#define PART_NAME_LENGTH 50
#define MAX_PORTS 28
#define MAX_PARTS 16
#define NUM_DEVICES 8
#define DEVICE_DATA_LENGTH 32
#define MAX_NODES 16

// This structure stores data about the mapping, such as the number of inputs
// or the total number of mux devices. Not all of these properties are required
// in all cases.
struct mapdata {
	int matrix_rows;
	int matrix_cols;
	int mux_count;
	int device_count;
	int nodes_per_row;
	int inputs_per_col;
};

// This structure stores a single device mapping
struct deviceMap {
	char name[PART_NAME_LENGTH]; // Name of the device
	int ports; // Total number of ports on device
	int portInput[MAX_PORTS]; // Which input each port is mapped to
};


// Stores the entire device mapping
// Includes an array of type deviceMap and metadata
struct deviceMapping {
	struct deviceMap* deviceMapList;
	int numDevices;
	int maxDevices;
};

struct nodeMapping {
	int device;
	int common;
};


int readMapData(struct mapdata* outMapdata, FILE * mapfile);
int readDeviceMapping(struct deviceMapping *outDeviceMapping, FILE * mapfile);
struct deviceMapping createDeviceMapping(int maxDevices);

