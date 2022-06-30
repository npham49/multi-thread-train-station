<img width="482" alt="image" src="https://user-images.githubusercontent.com/63203684/176584528-3cc357d6-08cb-4422-a99d-9a1b056c9c48.png">

INTRODUCTION

A simple train station simulator with multithreading, mutexes and conditional variables

INPUT

-a .txt file with the following format:
+each line represent a train with the first element being wither e,E,w,W. Second element is the load time with cross time being the 3rd element, separated by " "

OUTPUT

-if a train is ready to go/finished loading :%timestamp% Train %train_number% is ready to go %direction%
-if a train is on a track: %timestamp% Train %train_number% is ON the main track going %direction%
-if a train has finished crossing: %timestamp% Train %train_number% is OFF the main track after going %direction%

HIGH-LEVEL OVERVIEW

-the main() function reads the file line by line and add each train to an array of train, with each array element containing a conditional variable to signal crossing
-after a train is added to the array a thread is created for that train
-after all trains are added to the array, a signal is broadcasted for all trains to start loading
-there are 2 queues representing high priority trains and low priority trains
-after loading is completed, the trains are added to the appropriate queues
-the dispatcher checks is the high priority queue is empty, if not it dispatches all the trains in the queue 
-then it dispatches all the low priority trains
-once all trains are done, program shuts down

REFERENCE
-Code for enqueue and dequeue cited from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
