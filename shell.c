//Zane Burstein
//260461170

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

//problems regarding freeing args:
//freeing args does not work for some reason. It crashes on args[1] every time. I showed the issue to Jit and he could not figure out why it was happening

typedef struct node
{
	char *command;
	pid_t bPID;
	struct node *next;
} backgroundProcessID;



int getcmd(char *prompt, char *args[], int *background, char **historyLine)
{

	int length, j, i = 0;
	char *token, *loc;
	char *line, *temp;
	size_t linecap = 0;

	printf("%s", prompt); //print the prompt

	length = getline(&line, &linecap, stdin);//read in the users input


	//if the user did not enter anything
	if (length <= 0) 
		exit(-1);

	//pass the line to main function for future use by history
	temp = strdup(line);
	temp[length-1] = '\0';
	*historyLine = temp;

	// Check if background is specified. Checks if & was one of the param
	if ((loc = index(line, '&')) != NULL) 
	{
		*background = 1;
		*loc = ' ';
	}	
	else
		*background = 0;

	//tokkenizes the user input through while loop
	while ((token = strsep(&line, " \t\n")) != NULL)
	{
		for (j = 0; j < strlen(token); j++)
		{
			if (token[j] <= 32)
				token[j] = '\0';
			if (strlen(token) > 0)
			{
				args[i++] = token;
				break;
			}
		}
	}

	free(line);//free the buffer created by getline
	return i;
}


int main()
{
	char *args[20], *history[10];
	int bg, cnt, i, j, status, error, fgAccess, historyLocation, historyEntry = 0;
	pid_t pid;
	char *recentCommand, *token, *toBeTok, currentDirectory[50];
	backgroundProcessID* head; 
	backgroundProcessID* tail;
	backgroundProcessID* newNode;
	backgroundProcessID* temp;
	backgroundProcessID* delete;

	//creat the head and tail of the linkedlist that will hold the process ids of those running in background
	head = (backgroundProcessID*) malloc(sizeof(backgroundProcessID)); 	
	tail = (backgroundProcessID*) malloc(sizeof(backgroundProcessID));
	while (1)
	{
		cnt = getcmd("\n>>  ", args, &bg, &recentCommand);//get the command 
		args[cnt] = NULL;//set the last value of args as NULL to pass to execvp.

		//if usered attempted to reuse command in history
		if(strcmp(args[0], "r") == 0)
		{
			if(historyEntry == 0) //if history is empty
			{
				printf("Error: Command does not exist in history becasue history is empty\n");
				continue;
			}

			free(recentCommand);//need to free recent so it can be used againt to retrieve from history

			//if user just entered r
			if (cnt == 1)
			{
				if(historyEntry < 10)
					historyLocation = historyEntry - 1;
				else
					historyLocation = 9;
			}

			else //need to search through and find the right command
			{
				//if history is not full
				if(historyEntry < 10)
					for(historyLocation=historyEntry - 1; historyLocation >= 0; historyLocation--)
					{
						if(strncmp(history[historyLocation], args[1], 1) == 0)
							break;
					}
				//if history is full
				else
					for (historyLocation = 9; historyLocation >=  0; historyLocation--)
					{
						//if present
						if(strncmp(history[historyLocation], args[1], 1) == 0)
							break;
					}
			}

			recentCommand = strdup(history[historyLocation]);//duplicate one to add to the history
			toBeTok = strdup(history[historyLocation]); //duplicate one to be tokenized to populate args
			//freecmd(args, cnt); //args needs to be freed or else it causes a memory leak. However I am having issues with this command. See comment at top of program

			//Seperate the strings for use in args
			j = 0;
			token = (&toBeTok, " "); 
			while(token != NULL)  
			{
				if (token[j] > 32)
					args[j++] = token;
				token = strsep(&toBeTok, " ");
			}

			cnt = j;//reset the cnt
			args[j] = NULL; //set last value to NULL
			free(toBeTok); //free remaining toBeTok
		}


		//if user entered history, print out history
		if(strcmp(args[0], "history") == 0 && cnt == 1)
		{
			historyEntry = addHistory(history, recentCommand, historyEntry);//add to history
			if (historyEntry > 10) //when array is full
			{
				for (i = 0, j = 9; i < 10; i++, j--)
					printf("History Entry %d: %s\n", historyEntry - j, history[i]);
			}

			else //when array is not full
			{
				for(i = 0, j=1; i < historyEntry; i++, j++)
					printf("History Entry %d: %s\n", j, history[i]);
			}
		}


		//if user entered cd
		else if(strcmp(args[0], "cd") == 0)
		{
			error = chdir(args[1]);
			if(error == -1)
				perror("Error");//if error
			else
				historyEntry = addHistory(history, recentCommand, historyEntry);//add to history
		}

		//if user entered pwd
		else if (strcmp(args[0], "pwd") == 0) 
		{
			if(getcwd(currentDirectory, 50) != NULL)
			{
				printf("Current directory: %s\n", currentDirectory);
				historyEntry = addHistory(history, recentCommand, historyEntry);//add to history
			}
			else
				perror("Error ");//if error
		}

		else if(strcmp(args[0], "exit") == 0)
			exit(EXIT_SUCCESS);

		//if user entered jobs
		else if (strcmp(args[0], "jobs") == 0)
		{

			historyEntry = addHistory(history, recentCommand, historyEntry);//add command to history				

			//loop through the background ids and print them out or delete them if completed
			temp = head;
			i = 1;
			while(temp -> next != NULL)
			{

				//if the process has finished and needs to be removed from the linked list
				if(waitpid(temp->bPID, &status, WNOHANG) != 0)
				{
					//in the event that it is the head
					if(temp == head)
					{	
						head = head->next; //set new head
						free(temp); //free old one
						temp = head;//then move temp to new head to continue iteration
					}

					else
					{
						delete = head;
						while(delete->next != temp)//iterate through until before one to be deleted
							delete = delete->next;
						delete->next = temp->next;//delink
						free(temp);//delete it
						temp = delete->next;//move it to the next one
					}		
				}
				else //else its to be printed
				{
					printf("%d: %s\n", i++, temp->command);
					temp = temp -> next;
				}
			}
			//could be the head if only value  in jobs or the tail
			if(temp == head && head->bPID != '\0')
			{
				if(waitpid(temp->bPID, &status, WNOHANG) != 0)
				{	
					if(head == tail)
						tail = (backgroundProcessID*) calloc(1, sizeof(backgroundProcessID));//create new tail of null values
					free(head);//free the node
					head = (backgroundProcessID*) calloc(1, sizeof(backgroundProcessID));//create new head of null values. Need it initialized to 0s
					printf("No current active processes\n");
				}
				else
					printf("%d: %s\n", i, temp->command);
			} 	
			//check the tail
			else if(temp == tail)
			{
				if(waitpid(temp->bPID, &status, WNOHANG) != 0)
				{
					if(head->next == tail) //if only 2 values in jobs
					{	
						free(temp);//free the node
						tail = (backgroundProcessID*) malloc(sizeof(backgroundProcessID));//create new tail of null values
					}
					else //if more than two values
					{
						delete = head;
						while(delete->next != tail)//iterate until at new tail
							delete = delete->next;
						delete->next = NULL;//delink 
						free(tail);//delete old tail
						tail = delete;//set new one 
					}
				}
				else

					printf("%d: %s\n", i, temp->command);
			}

			else //if no values
				printf("No current processes\n");				
		}

		else if (strcmp(args[0], "fg") == 0)
		{

			if(cnt < 2)
			{
				printf("User needs to provide a number\n");
				continue;
			}
			temp = head;
			fgAccess = atoi(args[1]); //turn string into int	

			if (fgAccess >  0)//check to make sure it is an int
			{
				i = 1;
				while (i < fgAccess && temp->next != NULL)
				{
					temp = temp -> next;
					i++;
				}
				if(i == fgAccess)//if at proper location
				{
					waitpid(temp->bPID, &status, 0);//wait
					if(status == -1)
						perror("Error");
					else
						historyEntry = addHistory(history, recentCommand, historyEntry);//add to history
				}
				else//if jobs is empty
					printf("Invalid input. User must enter an integer corresponding to an active process process provided by command \"jobs\"\n");
			}
			else //if no int was provided
				printf("Invalid input. User must enter an integer greater than 1 corresponding to an active process proovided by command \"jobs\" \n");	 
		}

		else//not a built in command and need to fork
		{

			if(pid = fork()) //enters the parent
			{
				if (pid == -1)
				{
					perror("fork");
					continue;
				}

				if (bg)//if child is to run in background
				{
					historyEntry = addHistory(history, recentCommand, historyEntry);
					//if the linked list is currently empty
					if(head->bPID =='\0')
					{
						head->command = strdup(recentCommand);
						head->bPID = pid;
					}
					//if only one process in the background
					else if(tail->bPID == '\0' && head->next == NULL)
					{
						tail->command = strdup(recentCommand);
						tail->bPID = pid;
						head->next = tail;
					}
					//add for the rest
					else
					{
						newNode = (backgroundProcessID*) malloc(sizeof(backgroundProcessID));
						newNode->command = strdup(recentCommand);
						newNode -> bPID = pid;
						tail->next = newNode;
						tail = newNode;
					}

				}

				//if parent is to wait
				else
				{
					waitpid(pid, &status, 0);//wait on the id

					if ( status == -1) //if child exited with error
						perror("Error:");

					else if (status == 0)
						historyEntry = addHistory(history, recentCommand, historyEntry);//add command to history	
				}
				//freecmd(args, cnt);//need to free args at this point or will get memory leak. Issues with this. See comment at top of doc
				//do not want to free recent command becasue that string was not duplicated but instead placed within hsitory so it will not cause memory leak
			}

			else //child
			{
				error = execvp(args[0], args); //execute the command

				if(error == -1)//error check
				{
					perror("Error");	
					exit(EXIT_FAILURE);
				}

				else //if command is executed successfully
					exit(EXIT_SUCCESS);

			}
		}
	}
}

//frees arguments
freecmd(char *args[], int count)
{
	printf("after call to free args\n");
	int i;
	for(i = 0; i <  count; i++)
	{	
		printf("freeing args[%d]\n", i);
		free(args[i]);
	}
}

int addHistory(char *history[], char *recentCommand, int historyEntry)
{
	int i = 0;
	//add the command to history
	if (historyEntry < 10) //if history array not full
	{
		history[historyEntry] = recentCommand;
		historyEntry++;
	}
	else //if it is full
	{

		free(history[0]);//free the string from memory that is to be deleted from the history
		for(i = 0; i < 10; i++)//loop through and reassign the values to make space for the newest value on the right
			history[i] = history[i+1];
		history[9] = recentCommand;//then add the most recent command
		historyEntry++;
	}
	return historyEntry;
}
