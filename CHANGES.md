# Changes from original
___

- ### Custom dictionary's
   - Add a command line argument that allows users to add their own dictionary's.

- ### Error Color
   - Change the error color from light grey to RED

- ### Command line handler
   - Allow user to enter command line arguments by not parsing everything to less.

- ### Adding less cli flags automatically
   - using setenv to add flags to less on run.

- ### More CLI flags
   - Adding -i flag to ignore characters.
   - Adding -b flag for styling output.
   - Adding -u flag to underline mistakes.
   - Adding -l flag for line counting.
   - Adding -c flag for collum counting.
   - Adding -d flag for adding custom dictionarys.

- ### Improved parsing
   - The parser now sees words like "google-fu" as a word google-fu so can now be ignored.
   - Added parser.c and parser.h that contain all sorts of usefull functions for parsing text.
