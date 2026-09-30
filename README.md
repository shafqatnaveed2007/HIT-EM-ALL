# HIT 'EM ALL

A carnival-themed balloon shooting game developed from scratch using the C programming language and the Raylib library for our 1-1 semester project.

## Project Team

- Shafqat Naveed (2505048)
- Akaid Adnan (2505047)

## Supervisor

- Zim Mim Siddiqee Sir

## Prerequisites & Dependencies

To compile and run this project, you will need:
Visual Studio Code (VS Code) installed on your system.
The C/C++ extension for VS Code.
The Raylib template structure containing the necessary raylib/ library files and .vscode/ configuration files.

## Project Structure

Your project's top-level folder should contain:

main.c (Complete source code)

raylib/ (Containing all platform subfolders/headers/libraries)

.vscode/ (Containing pre-configured tasks.json and launch.json)

assets/
audio/ (Background music and sound effects)
sprites/ (Images, menus, characters, and balloons)
fonts/ (Carnival custom font)

leaderboard.txt (Auto-generated high score storage)

## Compilation & Running Instructions (VS Code)

Open the Project Folder:

Launch VS Code.
Go to File > Open Folder... and select the project's top-level folder (or open the VS Code workspace file).

Important: Do not open a single .c file on its own; the workspace requires the folder open to detect the .vscode/ settings.
Extensions: If prompted with "This workspace has extension recommendations," click Install to ensure you have the C/C++ extension enabled.

Build the Project:
Press Ctrl + Shift + B (Windows/Linux) or Cmd + Shift + B (Mac) and select Run Build Task.
This executes the pre-configured build task linked to your Raylib directory. Look for a "Build finished successfully" message with no errors in the terminal panel.

Run and Debug:
Open the Run and Debug panel (Ctrl + Shift + D on Windows/Linux or Cmd + Shift + D on Mac).
Press the green Play (▶) button or hit F5 to build and launch the game with the debugger attached.

## How to Play

Aim: Move your mouse to adjust the bow angle.
Pull & Shoot: Click and HOLD the left mouse button to pull back the bowstring, then RELEASE to fire your arrow.

Objectives:
Pop normal balloons for standard points.
Hit GOLD balloons to earn bonus arrows (+2) and extra points.
Avoid DANGER balloons — hitting them triggers an instant game over!
Clear POP balloons before they escape, or you will lose arrows!
