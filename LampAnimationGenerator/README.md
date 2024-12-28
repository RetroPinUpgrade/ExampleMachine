# Code to create lamp animations  

## Instructions  
Get GCC or another C compiler  
Create array of lamps for your machine (fill in the allLamps array in main.c). I use this site to record XY coordinates for my lamps: https://www.mobilefish.com/services/record_mouse_coordinates/record_mouse_coordinates.php     
Make a list of what animations you want to generate and put them in main()
```	GenerateRadarAnimation(312, 695, "Radar Animation");
	GenerateCenterOutAnimation(312, 695, "Center Out Animation");
	GenerateBottomToTopAnimation("Bottom to Top Animation");
	GenerateCenterOutAnimation(163, 869, "VUK Center Animation");
	GenerateCenterOutAnimation(406, 260, "Pop Bumper Center Animation");
```
Compile main.c and then run a.out to create animations.h
