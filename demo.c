/* This is a small app to help kids learn the multiplication table */

#include "tinyGUI/tinyGUI.h" /* Add tinyGUI to the project */

#define NUMBUTTONS 10 /* The number of the number buttons */
#define TASKLENGTH 27 /* The maximum length of the task text */

void numBtnOnClick(Button sender, void *context, MouseEventArgs e){ /* The number buttons' onClick event handler */
	TextBox textbox = (TextBox)context;
	int newTextLen;
	char *newText;

	newTextLen = strlen(*textbox->text) + 1;
	newText = (char*)malloc(newTextLen + 1); /* Allocate memory for the new text */
	if (!newText)
		MessageBoxA(NULL, "Out of memory!", "Error!", MB_OK);
	else {
		strcpy_s(newText, newTextLen + 1, *textbox->text);
		strcat_s(newText, newTextLen + 1, *sender->text); /* Concatenate the pressed button's text to the textbox's text */
		newText[newTextLen] = '\0';
		$(textbox)->setText(newText); /* Set the textbox's new text */
		free(newText); /* Free the new text. This is perfectly safe, the textbox's text is now stored elsewhere in memory */
	}
}

int setTask(Label task){ /* Sets a new random task in a label */
	int numA, numB;
	char newText[TASKLENGTH];

	srand((unsigned int)GetTickCount()); /* Generate 2 random numbers from 0 to 12 */
	numA = rand() % 13;
	numB = rand() % 13;

	sprintf_s(newText, TASKLENGTH, "%d MULTIPLY BY %d EQUALS", numA, numB);

	$(task)->setText(newText); /* Set the label's text to the new task */
	return numA * numB; /* return the task's solution */
}

void checkAnsBtnOnClick(Button checkAnsButton, void **context, MouseEventArgs e){ /* The "check answer" button's onClick asynchronous event handler */
	/* Get the objects needed by the event handler from the context */
	Label task = (Label)context[0], header = (Label)context[1];
	TextBox textbox = (TextBox)context[2];
	int *answer = (int*)context[3], answerInput = 0;

	sscanf_s(*textbox->text, "%d", &answerInput, 1); /* Get the numeric value of the answer input */

	startSync(header); /* Synchronize access to the header object */
	$(header)->setText((answerInput == *answer) ? "GOOD JOB!" : "WRONG ANSWER!"); /* Display the result */
	startSync(checkAnsButton); $(checkAnsButton)->setEnabled(FALSE); endSync(checkAnsButton); /* Disable the "check answer" button (synchronized) */
	Sleep(2000); /* Pause for 2000 milliseconds */
	$(header)->setText("Now can you calculate this?"); /* Pause is over, start new task */
	endSync(header);

	startSync(checkAnsButton); $(checkAnsButton)->setEnabled(TRUE); endSync(checkAnsButton); /* Enable the "check answer" button (synchronized) */
	startSync(textbox); $(textbox)->setText(""); endSync(textbox); /* Clear the textbox (synchronized) */
	startSync(task); *answer = setTask(task); endSync(task); /* Set a new task and save its answer to the context (synchronized) */
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
	/* Create the objects */
	Window window = newWindow(hInstance, "Learn the Multiplication Table!", 440, 200); /* Create the window */
	Label header = newLabel(hInstance, "Can you calculate this?", 400, 20, 10, 10), /* Create the header label */
			task = newLabel(hInstance, "", 220, 20, 10, 35); /* Create the task label */
	TextBox ansInput = newTextBox(hInstance, "", 102, 25, 240, 32, SINGLELINE); /* Create the answer textbox (a single-line textbox) */
	Button numberButtons[NUMBUTTONS], /* The number buttons */
			checkAnsButton = newButton(hInstance, "Check answer", 400, 25, 10, 110); /* Create the "check answer" button */

	char *buttonTexts[NUMBUTTONS] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	int i, firstTaskAnswer;
	void *buttonContext[4] = {(void*)task, (void*)header, (void*)ansInput, NULL}; /* Create the context for the "check answer" button's onClick event */
	
	firstTaskAnswer = setTask(task); /* Get the answer to the first random task */
	buttonContext[3] = (void*)&firstTaskAnswer; /* And add it to the context for the "check answer" button onClick event */

	$(window)->setResizable(FALSE); /* Disable resizing the window */ 
	$(window)->enableMaximize(FALSE); /* Disable its maximize box */
	$(ansInput)->setNumOnly(TRUE); /* Set the textbox to accept only numbers */
	$(checkAnsButton)->setOnClick((ButtonCallback)&checkAnsBtnOnClick, (void*)buttonContext, ASYNC); /* Set an asynchronous onClick event
																										for the "check answer" button */
	for (i = 0; i < NUMBUTTONS; i++){ /* Add the 10 number buttons */
		numberButtons[i] = newButton(hInstance, buttonTexts[i], 40, 40, 10 + i * 40, 60); /* Create a button */
		$(numberButtons[i])->setOnClick(&numBtnOnClick, (void*)ansInput, SYNC); /* Set a synchronous onClick event for the button */
		$(window)->addButton(numberButtons[i]); /* Add the button to the window */
	}

	$(window)->addLabel(header); /* Add the header label to the window */ 
	$(window)->addLabel(task); /* Add the task label to the window */
	$(window)->addTextBox(ansInput); /* Add the answer input textbox to the window */
	$(window)->addButton(checkAnsButton); /* Add the "check answer" button to the window */

	displayWindow(window, nCmdShow); /* Display the created window */

	for (i = 0; i < NUMBUTTONS; i++) /* The window has been closed, destroy the created objects */
		deleteButton(numberButtons[i]);
	deleteButton(checkAnsButton); deleteTextBox(ansInput); deleteLabel(task); deleteLabel(header); deleteWindow(window);

	return 0;
}
