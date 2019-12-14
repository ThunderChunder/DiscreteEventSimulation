#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <cstring>
#include <fstream>

using namespace std;

const int CUSTOMER_QUEUE_LENGTH = 500;
const int EVENT_QUEUE_LENGTH = 1000;
const int FILE_NAME_SIZE = 40;
const int ROOT_INDEX = 1;
const int MAX_AMOUNT_OF_SERVERS = 20;
const char fileName[] = "ass2.txt";

int globalServerSize = 0;

struct EventRecord;

int queueHead = 0;
int queueTail = 0;

int eventCount = 0;

int eventQueue[EVENT_QUEUE_LENGTH];
int heapEnd = 2;

int serverHeapEnd = 2;

double currentTime = 0;

struct EventRecord
{
	EventRecord()
	{
		//if type = -1, customer event
		//if type > -1, the number is the servers index in server struct
		this->type = -1;
		this->eTime = 0;
		this->tTime = 0;
	}
	void setCustomerEvent(double eTime, double tTime, char *pMethod)
	{
		this->eTime = eTime;
		this->tTime = tTime;
		strcpy(this->pMethod, pMethod);
	}
	void setServerEndEvent(double endTime, int serverNumber)
	{
		this->eTime = endTime;
		this->type = serverNumber;
	}
	void clearRecord()
	{
		this->type = -1;
		this->eTime = 0;
		this->tTime = 0;
		this->pMethod[0] = '\0';	
	}
	int type;
	double eTime;
	double tTime;
	char pMethod[40];
};

struct Server
{
	Server()
	{
		this->busy = false;
		this->efficiency = 0;
		this->totalServed = 0;
		this->timeIdle = 0;
		this->lastAction = 0;
	}
	bool busy;
	double efficiency;
	int totalServed;
	double timeIdle;
	double lastAction;
};

int cRecord[CUSTOMER_QUEUE_LENGTH];
EventRecord eRecord[EVENT_QUEUE_LENGTH];
Server server[MAX_AMOUNT_OF_SERVERS];
int serverHeap[MAX_AMOUNT_OF_SERVERS];

void swap(double &x, double &y)
{
	double z;

	z = x;
	x = y;
	y = z;
}

void dequeueCustomer(int *cRecord, int &queueHead, int &queueTail)
{
	if(queueHead < CUSTOMER_QUEUE_LENGTH)
	{
		cRecord[queueHead] = -1;
		queueHead++;
	}
	else
	{
		//loop back around
		queueHead = 0;
		cRecord[queueHead] = -1;
	}	
}

bool isQueueEmpty(int *cRecord, int queueHead)
{
	if(cRecord[queueHead] == -1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void enqueueCustomer(int *cRecord, int &queueTail, int eventIndex)
{
	if(queueTail < CUSTOMER_QUEUE_LENGTH)
	{
		cRecord[queueTail] = eventIndex;
		queueTail++;
	}
	else if(queueTail >= CUSTOMER_QUEUE_LENGTH)
	{
		queueTail = 0;
		cRecord[queueTail] = eventIndex;
		queueTail++;
	}
	else
	{
		cout<<"Queue full."<<endl;
	}
	cRecord[queueTail] = -1;

}

void insertHeapRoot(int *heapStart, int index)
{
	heapStart[ROOT_INDEX] = index;
}

void insertIntoHeap(int *heap, int index, int &heapEnding, int totalHeapLength)
{
	if(heapEnding < totalHeapLength)
	{
		heap[heapEnding] = index;
		heapEnding++;
	}
}

void siftUp(int *heap, int index, EventRecord *eRecord, Server *server)
{
	if(index == 1 || index == 0){return;}

	int parent = index/2;
	if(eRecord != NULL)
	{
		if(eRecord[heap[parent]].eTime < eRecord[heap[index]].eTime){return;}
		else
		{
			swap(heap[parent], heap[index]);
			siftUp(heap, parent, eRecord, NULL);
		}
	}
	else if(server != NULL)
	{
		if(server[heap[parent]].efficiency < server[heap[index]].efficiency){return;}
		else
		{
			swap(heap[parent], heap[index]);
			siftUp(heap, parent, NULL, server);
		}
	}
}

void siftDown(int *heap, int index, EventRecord *eRecord, Server *server)
{
	int child = index*2;

	if(eRecord != NULL)
	{
		if(child >= heapEnd || child+1 >= heapEnd){return;}
		if(eRecord[heap[child]].eTime > eRecord[heap[child+1]].eTime){child+=1;}
		if(eRecord[heap[index]].eTime > eRecord[heap[child]].eTime)
		{
			swap(heap[child], heap[index]);
			siftDown(heap, child, eRecord, NULL);
		}
	}
	else if(server != NULL)
	{
		if(child >= serverHeapEnd || child+1 >= serverHeapEnd){return;}
		if(server[heap[child]].efficiency > server[heap[child+1]].efficiency){child+=1;}
		if(server[heap[child]].efficiency < server[heap[index]].efficiency)
		{
			swap(heap[child], heap[index]);
			siftDown(heap, child, NULL, server);
		}
	}

}

void setCurrentTime(double time)
{
	currentTime = time;
}

void readFileIn(fstream &inStream)
{
	inStream.open(fileName);

	if(inStream.fail())
	{
		cerr << "Unable to open "<<fileName<<endl;
		exit(0);
	}	
}

int main()
{
	int eventPosition = 0;
	int serverSize;
	double endTime;
	int totalCustomerServed = 0;
	int longestCustomerQueueLength = 0;
	int currentQueueLength = 0;
	int customerQueueLengthSum = 0;
	int customerQueueRateCounter = 0;
	double customerWaitTimeSum = 0;
	int totalCustomerServedImmediatelly = 0;

	double buffer;
	double temp;
	char textBuffer[FILE_NAME_SIZE];

	fstream inStream;
	readFileIn(inStream);

	inStream >> serverSize;
	globalServerSize = serverSize;

	inStream >> temp;
	server[0].efficiency = temp;
	insertHeapRoot(serverHeap, 0);

	for(int i = 1; i < serverSize; i++)
	{
		inStream >> temp;
		server[i].efficiency = temp;
		insertIntoHeap(serverHeap, i, serverHeapEnd, serverSize+1);
		siftUp(serverHeap, serverHeapEnd-1, NULL, server);
	}
	serverSize++;

	inStream >> buffer >> temp >> textBuffer;
	eRecord[eventPosition].setCustomerEvent(buffer, temp, textBuffer);
	insertHeapRoot(eventQueue, eventPosition);
	eventPosition++;

	while(heapEnd > 1)
	{
		//sets global time to next event time
		setCurrentTime(eRecord[eventQueue[ROOT_INDEX]].eTime);

		//-1 = customer arrival
		if(eRecord[eventQueue[ROOT_INDEX]].type == -1)
		{
			//if a server is available
			if(server[serverHeap[ROOT_INDEX]].busy == false)
			{
				//accumulate server idle time
				server[serverHeap[ROOT_INDEX]].timeIdle += currentTime - server[serverHeap[ROOT_INDEX]].lastAction;
				totalCustomerServedImmediatelly++;
				endTime = eRecord[eventQueue[ROOT_INDEX]].tTime * server[serverHeap[ROOT_INDEX]].efficiency;
				//if payment method is cash
				if(eRecord[eventQueue[ROOT_INDEX]].pMethod[3] == 'h'){endTime += 0.3;}
				//else payment method is card
				else{endTime += 0.7;}
				//most efficient server is no longer idle
				server[serverHeap[ROOT_INDEX]].busy = true;
				eRecord[eventPosition].setServerEndEvent(endTime+currentTime, serverHeap[ROOT_INDEX]);
				//swap with priority queue heap end
				insertIntoHeap(eventQueue, eventPosition, heapEnd, EVENT_QUEUE_LENGTH);
				swap(eventQueue[ROOT_INDEX], eventQueue[heapEnd-1]);

				//disregard/remove heap end value as the event was processed
				heapEnd--;

				//siftdown to find new root of priority queue
				siftDown(eventQueue, ROOT_INDEX, eRecord, NULL);
				eventPosition++;

				//swap with server heap end
				swap(serverHeap[ROOT_INDEX], serverHeap[serverHeapEnd-1]);
				//disregard/remove heap end value since server is not available anymore
				serverHeapEnd--;
				//find new root, new server with highest efficiency
				siftDown(serverHeap, ROOT_INDEX, NULL, server);

			}
			//no servers available
			else
			{
				//customer enters the queue until server is available
				enqueueCustomer(cRecord, queueTail, eventQueue[ROOT_INDEX]);
				swap(eventQueue[ROOT_INDEX], eventQueue[heapEnd-1]);
				heapEnd--;
				siftDown(eventQueue, ROOT_INDEX, eRecord, NULL);

				//always check if queue length is now longest recorded
				currentQueueLength = queueTail - queueHead;
				if(currentQueueLength > longestCustomerQueueLength){longestCustomerQueueLength = currentQueueLength;}
				//used to calculate queue average
				customerQueueLengthSum += currentQueueLength;
				//used to calculate queue average
				customerQueueRateCounter++;
			}
			//if not end of file read in next customer
			//add the customer arrival event to priority queue
			if(!inStream.eof())
			{
				inStream >> buffer >> temp >> textBuffer;
				eRecord[eventPosition].setCustomerEvent(buffer, temp, textBuffer);
				insertIntoHeap(eventQueue, eventPosition, heapEnd, EVENT_QUEUE_LENGTH);
				siftUp(eventQueue, heapEnd-1, eRecord, NULL);
				eventPosition++;
			}
		}
		//server finish event
		else
		{
			totalCustomerServed++;
			//find the server whos event just finished
			//eRecord.type stores the servers index of the server struct array to point back to it
			//They are then set to available
			server[eRecord[eventQueue[ROOT_INDEX]].type].busy = false;
			//The server whos event just finished increments totalServed
			server[eRecord[eventQueue[ROOT_INDEX]].type].totalServed++;
			//record last action to work out idle time
			server[eRecord[eventQueue[ROOT_INDEX]].type].lastAction = currentTime;
			//Server gets added back into server heap to keep most efficient at the root
			insertIntoHeap(serverHeap, eRecord[eventQueue[ROOT_INDEX]].type, serverHeapEnd, serverSize);

			siftUp(serverHeap, serverHeapEnd-1, NULL, server);
			//swap with priority queue heap end
			swap(eventQueue[ROOT_INDEX], eventQueue[heapEnd-1]);
			//disregard/remove heap end value as the server finish event was processed
			heapEnd--;
			//siftdown to find new root of priority queue
			siftDown(eventQueue, ROOT_INDEX, eRecord, NULL);

			//process next customer in customer queue if its got someone in the queue
			if(!isQueueEmpty(cRecord, queueHead))
			{
				endTime = eRecord[cRecord[queueHead]].tTime * server[serverHeap[ROOT_INDEX]].efficiency;
				//store accumulative sum of total time all customers spent in the queue
				customerWaitTimeSum += currentTime - eRecord[cRecord[queueHead]].eTime;
				//if payment method is cash
				if(eRecord[cRecord[queueHead]].pMethod[3] == 'h'){endTime += 0.3;}
				//else payment method is card
				else{endTime += 0.7;}
				//most efficient server in heap is assigned to next customer
				server[serverHeap[ROOT_INDEX]].busy = true;
				server[serverHeap[ROOT_INDEX]].timeIdle += currentTime - server[serverHeap[ROOT_INDEX]].lastAction;
				//server end event put into heap
				eRecord[eventPosition].setServerEndEvent(endTime+currentTime, serverHeap[ROOT_INDEX]);
				dequeueCustomer(cRecord, queueHead, queueTail);
				//insert new server finish event in priority queue
				insertIntoHeap(eventQueue, eventPosition, heapEnd, EVENT_QUEUE_LENGTH);
				siftUp(eventQueue, heapEnd-1, eRecord, NULL);
				eventPosition++;

				//swap with server heap end
				swap(serverHeap[ROOT_INDEX], serverHeap[serverHeapEnd-1]);
				//disregard/remove heap end value since server is not available anymore
				serverHeapEnd--;
				//find new root, new server with highest efficiency
				siftDown(serverHeap, ROOT_INDEX, NULL, server);

				currentQueueLength = queueTail - queueHead;
				customerQueueLengthSum += currentQueueLength;
				customerQueueRateCounter++;
			}
		}
	}
	totalCustomerServed--;
	cout<<"Total customers served: "<<totalCustomerServed<<endl;
	cout<<"First customer arrived: "<<eRecord[0].eTime<<" and the last customer was served at: "<<currentTime<<endl;
	cout<<'\t'<<"Therefore time to server all customers: "<<currentTime-eRecord[0].eTime<<endl;
	cout<<"Longest Customer Queue length: "<<longestCustomerQueueLength<<endl;
	cout<<"Customer queue length average: "<<customerQueueLengthSum/customerQueueRateCounter<<endl;
	cout<<"Average time customer spent in queue: "<<customerWaitTimeSum/customerQueueRateCounter<<endl;
	cout<<"Percent of customers who were served immediately: "<<((double)totalCustomerServedImmediatelly/(double)totalCustomerServed)*100<<"%"<<endl;
	cout<<"Server stats: "<<endl;
	for(int i = 0; i < serverSize-1; i++)
	{
		cout<<'\t'<<" Server "<<i<<" served "<<server[i].totalServed<< " customers ";
		cout<<"and was idle for: "<<server[i].timeIdle<<endl;
	}
}


/*
	2 struct arrays were used, EventRecord and Server, EventRecord eRecord[] stores all event data that occurs, so customer arrivals and server finish events. Server server[] stores all the server attributes data.
	A priority queue is implemented (min heap) to keep EventRecords earliest event which is what should happen next, either next customer arrived or server event finished.
		The min heap is an int eventQueue[] that only stores EventRecords struct array index so only integer values are ever swapped in the heap sifts (indexes are used as pointers to struct storage)
	A priority queue is implemented (max heap) to keep the most efficient at the root, so the most efficient server is always selected at teh given time.
		The max heap is an int serverHeap[] that only stores Server struct array index so only integer values are ever swapped in the heap sifts (indexes are used as pointers to struct storage)
	A FIFO Queue is used to store customers who are not served immediately, this is an int cRecord[] that only enqueues the index to EventRecord eRecord[] so only integers are written into the queue (indexes are used as pointers to struct storage)
	All data structures (int eventQueue[], int serverHeap[] and int cRecord[]) store indexes of EventRecord eRecord[] to point to the associated data.

	EventRecord eRecord[]->type is used to distinguisg between customer arrival event and server finish event, if(type == -1) customer arrival process accordingly, if(type > -1) the integer value points to Server server[] struct to keep track
	of what server just finished and must be added back into the int serverHeap[] as they are now available again.
*/
