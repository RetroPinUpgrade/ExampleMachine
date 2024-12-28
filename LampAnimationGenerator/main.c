#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.141592654
#define LAMP_ANIMATION_STEPS	24
#define LINE_STEPS		1000
#define NUM_LAMPS		48
#define NUM_LAMP_BYTES		6
#define MAX_LAMP_ANIMATIONS	32
#define MAX_NAME_LENGTH		256



struct position {
int x;
int y;
};

int NumLampAnimationsUsed = 0;
unsigned char LampAnimations[MAX_LAMP_ANIMATIONS][LAMP_ANIMATION_STEPS][NUM_LAMP_BYTES];
unsigned char LampORs[MAX_LAMP_ANIMATIONS][NUM_LAMP_BYTES];
unsigned char AllLampBits[NUM_LAMP_BYTES];
char LampAnimationNames[MAX_LAMP_ANIMATIONS][MAX_NAME_LENGTH];
double LampGridDistance;


struct position allLamps[NUM_LAMPS] = {

{261,557},
{462,176},
{73,962},
{326,975},
{200,585},

{429,162},
{163,1058},
{500,962},
{50,333},
{351,505},

{312,695},
{550,963},
{406,261},
{385,487},
{327,769},

{0, 0},
{0, 0},
{324,162},
{296,1025},
{0, 0},

{406,260},
{0, 0},
{0, 0},
{0, 0},
{0, 0},

{0, 0},
{296,923},
{0, 0},
{405,261},
{0, 0},

{0, 0},
{0, 0},
{0, 0},
{485,201},
{163,869},

{312,1198},
{164,604},
{317,524},
{128,910},
{455,845},

{0, 0},
{501,231},
{295,821},
{457,1058},
{127,623},

{190,651},
{120,961},
{326,872}

};




int minX, minY, maxX, maxY;
int usedWidth, usedHeight;

void DetermineBoundsOfData() {
	minX = 32767;
	minY = 32767;
	maxX = -32767;
	maxY = -32767;

	unsigned char curLampByte = 0x00;

	int i;
	for (i=0; i<NUM_LAMPS; i++) {
		if (!(allLamps[i].x==0 && allLamps[i].y==0)) {
			if (allLamps[i].x<minX) minX = allLamps[i].x;
			if (allLamps[i].y<minY) minY = allLamps[i].y;
			if (allLamps[i].x>maxX) maxX = allLamps[i].x;
			if (allLamps[i].y>maxY) maxY = allLamps[i].y;
			curLampByte |= (0x01<<(i%8));
		}

		if ((i%8)==7) {
			AllLampBits[i/8] = curLampByte;
			curLampByte = 0;
		}
	}

	if ((i%8)) {
		AllLampBits[i/8] = curLampByte;
	}

	usedWidth = maxX-minX;
	usedHeight = maxY-minY;

	LampGridDistance = 5.0;
}


// Center of crosshairs = 233, 477

#define CENTER_LAMP	27

void setBitOn(unsigned char *lamps, int x, int y) {
//	int shiftedX = x + allLamps[CENTER_LAMP].x;
//	int shiftedY = y + allLamps[CENTER_LAMP].y;
	int shiftedX = x + 233;
	int shiftedY = y + 477;

	// see if any lights are within 10 points
	for (int i=0; i<NUM_LAMPS; i++) {
//		if (y<100) continue;
		if (allLamps[i].y>0) {
			int distance = ((allLamps[i].x-shiftedX)*(allLamps[i].x-shiftedX)) +
					((allLamps[i].y-shiftedY)*(allLamps[i].y-shiftedY));
			if (distance<100) {
				lamps[i/8] |= (1<<(i%8));
//				if (i==12) printf("%d, %d\n", x, y);
			}
		}
	}

}



void setBitOn1(unsigned char *lamps, int x, int y, int centerX, int centerY) {
	int shiftedX = x + centerX;
	int shiftedY = y + centerY;

	// see if any lights are within 10 points
	for (int i=0; i<NUM_LAMPS; i++) {
//		if (y<100) continue;
		if (allLamps[i].y>0) {
			int distance = ((allLamps[i].x-shiftedX)*(allLamps[i].x-shiftedX)) +
					((allLamps[i].y-shiftedY)*(allLamps[i].y-shiftedY));
			if (distance<100) {
				lamps[i/8] |= (1<<(i%8));
//				if (i==12) printf("%d, %d\n", x, y);
			}
		}
	}

}


void setBitOnAbsolute(unsigned char *lamps, int x, int y, double lampDistance) {

	// see if any lights are within 10 points
	for (int i=0; i<NUM_LAMPS; i++) {
		if (allLamps[i].x==0 && allLamps[i].y==0) continue;
		double distance = sqrt((double)(allLamps[i].x-x)*(allLamps[i].x-x) + (double)(allLamps[i].y-y)*(allLamps[i].y-y));
		if (distance<lampDistance) {
			lamps[i/8] |= (1<<(i%8));
		}
	}

}



void GenerateRadarAnimation(int centerX, int centerY, char *animationName) {
	if (NumLampAnimationsUsed>=MAX_LAMP_ANIMATIONS) {
		printf("Out of space for animations.\n");
	}
	printf("Generating Radar Animation: ");
	printf("%s", animationName);
	printf("\n");

	strcpy(LampAnimationNames[NumLampAnimationsUsed], (const char *)animationName);

	double x, y;
	double angle = 0.0;
	double radius = 0.0;
	int curStep = 0;

	unsigned char lamps[NUM_LAMP_BYTES];
	for (int i=0; i<NUM_LAMP_BYTES; i++) LampORs[NumLampAnimationsUsed][i] = 0;

	for (int count=0; count<LAMP_ANIMATION_STEPS*10; count++) {
		if ((count%10)==0) {
			for (int i=0; i<NUM_LAMP_BYTES; i++) lamps[i] = 0;
		}

		angle = (2.0 * PI / (LAMP_ANIMATION_STEPS*10)) * (double)count;
		x = (((double)usedWidth) * cos(angle))/(double)LINE_STEPS;
		y = (((double)usedHeight) * sin(angle))/(double)LINE_STEPS;

		for (int i=0; i<LINE_STEPS; i++) {
			setBitOn1(lamps, (int)(x*(double)i), (int)(y*(double)i), centerX, centerY);
		}

		if ((count%10)==9) {
			// Every 10th step, we can record the lamps on
			for (int i=0; i<NUM_LAMP_BYTES; i++) {
				LampAnimations[NumLampAnimationsUsed][curStep][i] = lamps[i];
				LampORs[NumLampAnimationsUsed][i] |= lamps[i];
			}
			curStep += 1;
		}
	}

	NumLampAnimationsUsed += 1;
}


void GenerateCenterOutAnimation(int centerX, int centerY, char *animationName) {
	if (NumLampAnimationsUsed>=MAX_LAMP_ANIMATIONS) {
		printf("Out of space for animations.\n");
	}
	printf("Generating Center Out Animation: ");
	printf("%s", animationName);
	printf("\n");

	strcpy(LampAnimationNames[NumLampAnimationsUsed], (const char *)animationName);

	unsigned char lamps[NUM_LAMP_BYTES];
	double x, y;
	double angle = 0.0;
	double radius = 0.0;
	int curStep = 0;

	double radiusScaleFactor;

	double minDistance = 999999.0;
	double maxDistance = 0.0;
	// Figure out distances of closest and farthest lamps
	for (int i=0; i<NUM_LAMPS; i++) {
		if (allLamps[i].x==0 && allLamps[i].y==0) continue;
		double curDistance = sqrt((allLamps[i].x-centerX)*(allLamps[i].x-centerX) + (allLamps[i].y-centerY)*(allLamps[i].y-centerY));
		if (curDistance>maxDistance) maxDistance = curDistance;
		if (curDistance<minDistance) minDistance = curDistance;
	}
	radiusScaleFactor = (maxDistance-minDistance) / ((double)(LAMP_ANIMATION_STEPS*10-1)*(double)(LAMP_ANIMATION_STEPS*10-1) + 0.75*((double)LAMP_ANIMATION_STEPS*10-1));

	for (int i=0; i<NUM_LAMP_BYTES; i++) LampORs[NumLampAnimationsUsed][i] = 0;

	for (int count=0; count<LAMP_ANIMATION_STEPS*10; count++) {
		if ((count%10)==0) {
			for (int i=0; i<NUM_LAMP_BYTES; i++) lamps[i] = 0;
		}

		radius = radiusScaleFactor*((double)count * (double)count + 0.75*(double)count) + minDistance;

		for (int i=0; i<LINE_STEPS; i++) {
			angle = ((double)i/(double)LINE_STEPS) * 2.0 * PI;
			x = radius * cos(angle);
			y = radius * sin(angle);
			setBitOnAbsolute(lamps, (int)(x) + centerX, (int)(y) + centerY, LampGridDistance);
		}

		if ((count%10)==9) {
			// Every 10th step, we can record the lamps on
			for (int i=0; i<NUM_LAMP_BYTES; i++) {
				LampAnimations[NumLampAnimationsUsed][curStep][i] = lamps[i];
				LampORs[NumLampAnimationsUsed][i] |= lamps[i];
			}
			curStep += 1;
		}
	}

	NumLampAnimationsUsed += 1;
}


void GenerateBottomToTopAnimation(char *animationName) {
	if (NumLampAnimationsUsed>=MAX_LAMP_ANIMATIONS) {
		printf("Out of space for animations.\n");
	}
	printf("Generating Bottom to Top Animation: ");
	printf("%s", animationName);
	printf("\n");

	strcpy(LampAnimationNames[NumLampAnimationsUsed], (const char *)animationName);

	unsigned char lamps[NUM_LAMP_BYTES];
	for (int i=0; i<NUM_LAMP_BYTES; i++) lamps[i] = 0;

	double stepHeight = ((double)(maxY-minY))/((double)LAMP_ANIMATION_STEPS);
	int curStep = 0;

	for (int count=0; count<LAMP_ANIMATION_STEPS; count++) {
		int minStepY = maxY - (int)(((double)(count+1))*stepHeight);
		int maxStepY = maxY - (int)(((double)(count))*stepHeight);
		for (int i=0; i<NUM_LAMP_BYTES; i++) lamps[i] = 0;

		for (int i=0; i<NUM_LAMPS; i++) {
			if (allLamps[i].y>=minStepY && allLamps[i].y<=maxStepY) {
				lamps[i/8] |= (1<<(i%8));
			}
		}

		for (int i=0; i<NUM_LAMP_BYTES; i++) {
			LampAnimations[NumLampAnimationsUsed][curStep][i] = lamps[i];
			LampORs[NumLampAnimationsUsed][i] |= lamps[i];
		}
		curStep += 1;
	}

	NumLampAnimationsUsed += 1;
}

#define NUM_RAIN_COLUMNS	16
void GenerateRainAnimation(char *animationName) {

	if (NumLampAnimationsUsed>=MAX_LAMP_ANIMATIONS) {
		printf("Out of space for animations.\n");
	}
	printf("Generating Rain Animation: ");
	printf("%s", animationName);
	printf("\n");

	strcpy(LampAnimationNames[NumLampAnimationsUsed], (const char *)animationName);

	int rainColumns[NUM_RAIN_COLUMNS];
	int rainColumns2[NUM_RAIN_COLUMNS];

	for (int dropCount=0; dropCount<NUM_RAIN_COLUMNS; dropCount++) {
		rainColumns[dropCount] = rand()%LAMP_ANIMATION_STEPS;
		rainColumns2[dropCount] = (rainColumns[dropCount] + LAMP_ANIMATION_STEPS/2 - 2 + rand()%5)%LAMP_ANIMATION_STEPS;
	}

	unsigned char lamps[NUM_LAMP_BYTES];

	for (int frameCount=0; frameCount<LAMP_ANIMATION_STEPS; frameCount++) {
		for (int i=0; i<NUM_LAMP_BYTES; i++) lamps[i] = 0;

		int dropX, firstDropY, secondDropY;
		for (int dropCount=0; dropCount<NUM_RAIN_COLUMNS; dropCount++) {
			dropX = minX + dropCount * (int)((double)(maxX-minX) / (double)(NUM_RAIN_COLUMNS-1));
			firstDropY = minY + (int)((double)rainColumns[dropCount] * ((double)(maxY-minY) / (double)(LAMP_ANIMATION_STEPS-1)));
			setBitOnAbsolute(lamps, dropX, firstDropY, 0.5 * ((double)(maxX-minX) / (double)(NUM_RAIN_COLUMNS-1)));
			secondDropY = minY + (int)((double)rainColumns2[dropCount] * ((double)(maxY-minY) / (double)(LAMP_ANIMATION_STEPS-1)));
			setBitOnAbsolute(lamps, dropX, secondDropY, 0.5 * ((double)(maxX-minX) / (double)(NUM_RAIN_COLUMNS-1)));
		}

		for (int i=0; i<NUM_LAMP_BYTES; i++) {
			LampAnimations[NumLampAnimationsUsed][frameCount][i] = lamps[i];
			LampORs[NumLampAnimationsUsed][i] |= lamps[i];
		}

		for (int dropCount=0; dropCount<NUM_RAIN_COLUMNS; dropCount++) {
			rainColumns[dropCount] = (rainColumns[dropCount]+1)%LAMP_ANIMATION_STEPS;
			rainColumns2[dropCount] = (rainColumns2[dropCount]+1)%LAMP_ANIMATION_STEPS;
		}
	}


	NumLampAnimationsUsed += 1;
}


unsigned char CountBits(unsigned int intToBeCounted) {
  unsigned char numBits = 0;

  for (unsigned char count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}


void WriteAnimationArrays(FILE *fptr) {

	fprintf(fptr, "// Lamp animation arrays\n");
	fprintf(fptr, "#define NUM_LAMP_ANIMATIONS       %d\n", NumLampAnimationsUsed);
	fprintf(fptr, "#define LAMP_ANIMATION_STEPS      %d\n", LAMP_ANIMATION_STEPS);
	fprintf(fptr, "#define NUM_LAMP_ANIMATION_BYTES  %d\n", NUM_LAMP_BYTES);
	fprintf(fptr, "byte LampAnimations[NUM_LAMP_ANIMATIONS][LAMP_ANIMATION_STEPS][NUM_LAMP_ANIMATION_BYTES] = {\n");

	for (int animCount=0; animCount<NumLampAnimationsUsed; animCount++) {
		fprintf(fptr, "  // %s (index = %d)\n", LampAnimationNames[animCount], animCount);
		fprintf(fptr, "  {\n");

		unsigned char lampsOnThisStep = 0;

		for (int stepCount=0; stepCount<LAMP_ANIMATION_STEPS; stepCount++) {
			if (stepCount>0) {
				fprintf(fptr,"}, // lamps on = %d\n", lampsOnThisStep);
			}
			lampsOnThisStep = 0;
			fprintf(fptr, "    {");
			for (int byteCount=0; byteCount<NUM_LAMP_BYTES; byteCount++) {
				if (byteCount>0) fprintf(fptr, ", ");
				fprintf(fptr, "0x%02X", LampAnimations[animCount][stepCount][byteCount]);
				lampsOnThisStep += CountBits(LampAnimations[animCount][stepCount][byteCount]);
			}
		}
		fprintf(fptr,"}  // lamps on = %d\n", lampsOnThisStep);

		fprintf(fptr,"  // Bits Missing\n");
		fprintf(fptr,"  // ");
		for (int byteCount=0; byteCount<NUM_LAMP_BYTES; byteCount++) {
			if (byteCount>0) fprintf(fptr, ", ");
			fprintf(fptr, "0x%02X", AllLampBits[byteCount] & ~LampORs[animCount][byteCount]);
		}
		fprintf(fptr,"\n");

		fprintf(fptr,"  }");
		if (animCount<(NumLampAnimationsUsed-1)) fprintf(fptr,",\n");
		else fprintf(fptr,"\n");
	}
	fprintf(fptr,"};\n");

}

int main() {

	DetermineBoundsOfData();
	GenerateRadarAnimation(312, 695, "Radar Animation");
	GenerateCenterOutAnimation(312, 695, "Center Out Animation");
	GenerateBottomToTopAnimation("Bottom to Top Animation");
	GenerateCenterOutAnimation(163, 869, "VUK Center Animation");
	GenerateCenterOutAnimation(406, 260, "Pop Bumper Center Animation");

	FILE *fptr;
	fptr = fopen("animations.h", "w");

	if(fptr == NULL) {
	 printf("Error opening file for writing!");
	 exit(1);
	}

	WriteAnimationArrays(fptr);

	fclose(fptr);

}
