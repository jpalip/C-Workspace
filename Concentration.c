#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


typedef struct Card {
    char text[3];
    int num;
} Card; 

int num1;
int num2;
int go = 0;
int score = 0;
int tries = 0;
int match = 0;
int matched[16] = {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
struct Card *board[16];

// Shuffles the 0-15 number array, to assign random texts from predefined list into board array
void shuffle(int arr[], int size) {
    srand(time(NULL));
    int i;
    for(i = size-1; i > 0; i--) { // swaps the elements in the array
        int j = rand() % (i+1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

// Creates the cards and puts them into the board
void createCards(struct Card *board[16]) {
    char *texts[] = {"dog", "cat", "rat", "hat", "bat", "rap", "dog", "cat", "rat", "hat", "bat", "rap", "tap", "tap", "bet", "bet"};
    int arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    int size = 16;
    shuffle(arr, size);
    for(int i = 0; i < size; i++) {
        struct Card *card = (struct Card*)malloc(sizeof(Card));
        strcpy(card->text, texts[arr[i]]); 
        card->num = i; 
        board[i] = card;
    }
    printf("\n");
}

// Sets up the game, asks user to confirm game
void initialization() {
    printf("PLAY  | This is a game of concentration. There are 16 cards each filled with random text, every 2 cards contain the same text.");
    printf("\nPLAY  | The goal is to match up all the cards with the lowest amount of tries. When you match all the cards the game restarts.");
    printf("\nPLAY  | Enter 1 to Play or 0 to Quit: ");
    scanf("%d", &go);
    if(go == 1) {
        printf("START | * Setting up the game *\n");
        createCards(board);
    }
}

// DEBUG METHOD - Only used to test program
void printCards() {
    for(int i = 0; i < 16; i++) {
        printf("%s = %d, ", board[i]->text, board[i]->num);
    }
    printf("\n");
}

void teardown() {
    for(int i = 0; i < 16; i++) {
        free(board[i]);
    }
    printf("END   | * Destroying the game and quitting. *\n");
}

int alreadyMatch(int num) {

    for(int i = 0; i < 16; i++) {
        if((num-1) == matched[i]) {
            return 1;
        }
    }
    return 0;
}

// Helper method to ensure good input
void errorHandling() {
    if(num1 != 0 || num2 != 0) {
        while((num1 < 0 || num1 > 17) || (num2 < 0 || num2 > 17) || num2 == num1) {
            if(num2 == num1) {
                printf("ERROR | Enter unique numbers: ");
                scanf("%d", &num2);
            }
            if((num1 < 0 || num1 > 17)) {
                printf("ERROR | Expected number between 1-16: ");
                scanf("%d", &num1);
            }
            if(num2 < 0 || num2 > 17) {
                printf("ERROR | Expected number between 1-16: ");
                scanf("%d", &num2); 
            }
            if((num1 == 0 || num1 < 0 || num1 > 17) || (num2 < 0 || num2 > 17)) {
                go = 0;
            }
        }
        while(alreadyMatch(num1) == 1 || alreadyMatch(num2) == 1) {
            if(alreadyMatch(num1) == 1) {
                printf("ERROR | Enter number 1 already matched: ");
                scanf("%d", &num1);
            }
            if(alreadyMatch(num2) == 1) {
                printf("ERROR | Enter number 2 already matched: ");
                scanf("%d", &num2);
            }
        }      
    }
    else {
        go = 0;
    }   
}

// Collects user data for match
void acceptInput() {
    printf("INPUT | Enter your first number: ");
    scanf("%d", &num1);
    errorHandling();
    if(go == 1) {
        printf("INPUT | Enter your second number: ");
        scanf("%d", &num2);
        if(num2 == 0) {
            go = 0;
        }
        else 
            errorHandling();
        tries++;
        }
}


// Displays if data is match, and the current score
void DisplayWorld() {
    // Prints Match Results showing cards #/text
    if(match == 1)
    {
        printf("\t%d & %d\n", num1, num2);
        printf("\tMATCH!\n");
    }
    else {
        printf("\t%d & %d\n", num1, num2);
        printf("\t%s & %s\n", board[num1-1]->text, board[num2-1]->text);
        printf("\tNO MATCH!\n");
    }
    // Checks if game is over, promts new game
    if(score == 8) {
        printf("\tMatches: %d", score);
        printf("\tFinal Score: %d\n", tries);
        printf("END   | You won! Would you like to play again? (1 - YES)(0 - NO): ");
        scanf("%d", &go);
        if(go == 1) {
            createCards(board);
            for(int i = 0; i < 16; i++) {
                matched[i] = 20;
            }
            tries = 0;
            score = 0;
        }
    }
    // If game is not over, display game stats
    else {
        printf("\tTries: %d\n", tries);
        printf("\tMatches: %d\n", score);
        printf("\tRemaining Cards:\n\t");
        for(int i = 0; i < 16; i++) { // Formats board into 4x4 array printed
            if(matched[i] == 20)
                printf("%d, ", i + 1);
            else
                printf("X, ");
            if((i+1) % 4 == 0)
                printf("\n\t");
        }
        printf("\n");
    }
    num1 = 0;
    num2 = 0;
}

// Validates matches correctly - ensuring the texts matches
void UpdateWorld(int one, int two) {
    if(strcmp(board[one-1]->text, board[two-1]->text) == 0) {
        match = 1;
        // Places match card #s in array to prevent reuse 
        matched[num1-1] = (num1-1);
        matched[num2-1] = (num2-1);
        score++; // increase score for match
    }
    else {
        match = 0;
    }
    // Display after updating values
    DisplayWorld(board);
}

int main(int argc, char** argv)
{
    // Initializes the game setup
    initialization();

    while(go == 1) {
        acceptInput(); // Collects user input
        if(go == 1) {
            // Checks if card texts matches, then displays results
            // If max score is reached, promts user to restart or not
            UpdateWorld(num1, num2);
        }
    }
    // End Of Game, free all memory
    teardown(board);
    return 0;
}