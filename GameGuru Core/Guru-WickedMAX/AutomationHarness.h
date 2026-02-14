#ifndef AUTOMATION_HARNESS_H
#define AUTOMATION_HARNESS_H

// Automation Harness for Claude Code testing
// File-based command/response protocol
// Command file:  auto_command.txt
// Response file:  auto_result.txt
// Log file:       auto_log.txt

void AutoHarness_CheckForCommand(void);

// Force a welcome tab selection from automation (set to tab index, -1 = no force)
// 0=Demo Games, 1=My Games, 3=Tutorials, 4=User Guide, 5=Live Changelog
// 6=Workshop Uploader, 7=Workshop, 42=Community Tutorials
extern int g_iAutoForceWelcomeTab;

// Force a demo game selection from automation (set to display name, empty = no force)
extern char g_sAutoSelectDemo[260];

#endif
