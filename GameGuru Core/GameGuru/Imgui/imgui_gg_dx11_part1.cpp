const char *noc_file_dialog_open(int flags,
	const char *filters,
	const char *default_path,
	const char *default_name,
	bool bUseDefaultPath,
	const char *pTitle)
{
	OPENFILENAMEA ofn;       // common dialog box structure
	char szFile[MAX_PATH];       // buffer for file name
	int ret;
	szFile[0] = '\0';

	if (flags & NOC_FILE_DIALOG_DIR) 
	{
		static wchar_t lBuff[MAX_PATH];
		//wchar_t aTitle[MAX_PATH];
		BROWSEINFOW bInfo;
		LPITEMIDLIST lpItem;
		HRESULT lHResult;

		CoUninitialize();
		lHResult = CoInitialize(NULL);

		ZeroMemory(&bInfo, sizeof(BROWSEINFO));

		bInfo.hwndOwner = g_agkhWnd;
		bInfo.lpszTitle = L"Select folder";
		bInfo.lpfn = BrowseCallbackProc;

		if (lHResult == S_OK || lHResult == S_FALSE)
		{
			bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE; //BIF_USENEWUI; //BIF_NEWDIALOGSTYLE

			if (bUseDefaultPath && default_path)
			{
				// ZJ: Added MultiByteToWideChar so cast to LPARAM is successful
				wchar_t szFolderPath[MAX_PATH];
				MultiByteToWideChar(CP_UTF8, 0, default_path, -1, szFolderPath, MAX_PATH);
				bInfo.lParam = (LPARAM)szFolderPath;

				//bInfo.lParam = (LPARAM)default_path;
			}
				
			else
				bInfo.lParam = (LPARAM)NULL;

			lpItem = SHBrowseForFolderW(&bInfo);
			if (lpItem)
			{
				SHGetPathFromIDListW(lpItem, lBuff);
			}

			if (lHResult == S_OK || lHResult == S_FALSE)
			{
				CoUninitialize();
				CoInitializeEx(NULL, COINIT_MULTITHREADED);

			}
		}
		sprintf(szFile, "%ws", lBuff);

		//Make sure ther blocking dialog did not skip some keys, reset.
		ImGuiIO& io = ImGui::GetIO();
		io.KeySuper = false;
		io.KeyCtrl = false;
		io.KeyAlt = false;
		io.KeyShift = false;

		io.KeysDown[13] = false; //also reset imgui keys.
		io.KeysDown[16] = false;
		io.KeysDown[17] = false;
		io.KeysDown[18] = false;
		io.KeysDown[19] = false;
		io.KeysDown[0x7B] = false; //F12
		io.KeysDown[78] = false;
		io.KeysDown[79] = false;
		io.KeysDown[83] = false;
		io.KeysDown[90] = false;
		io.KeysDown[73] = false; // I reset all system wide shortcuts used.

		io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
		io.KeysDown[78] = false;
		io.KeysDown[76] = false;
		io.KeysDown[32] = false;

		io.MouseDown[0] = 0; //PE: Mouse (release) is also loast inside blocking dialogs. Reset!
		io.MouseDown[1] = 0;
		io.MouseDown[2] = 0;
		io.MouseDown[3] = 0;

		return strdup(szFile);
	}

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filters;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrTitle = pTitle;

	//PE: Why was default path removed ? added a flag instead bUseDefaultPath , need it :)
	if ((flags & NOC_FILE_DIALOG_OPEN || flags & NOC_FILE_DIALOG_SAVE) && default_path && bUseDefaultPath) {
		ofn.lpstrInitialDir = default_path;
	}
	else
		ofn.lpstrInitialDir = NULL;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.hwndOwner = g_agkhWnd;

	if (flags & NOC_FILE_DIALOG_DIR) 
	{
		ofn.Flags = OFN_CREATEPROMPT;
		ret = GetOpenFileNameA(&ofn);
	}
	else if (flags & NOC_FILE_DIALOG_OPEN) // || flags
		ret = GetOpenFileNameA(&ofn);
	else
		ret = GetSaveFileNameA(&ofn);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false; //also reset imgui keys.
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (g_noc_file_dialog_ret != NULL)
		free(g_noc_file_dialog_ret);
	g_noc_file_dialog_ret = ret ? strdup(szFile) : NULL;

	return g_noc_file_dialog_ret;
}

#else
#ifdef AGK_MACOS

#include <AppKit/AppKit.h>

const char *noc_file_dialog_open(int flags,
	const char *filters,
	const char *default_path,
	const char *default_name)
{
	NSURL *url;
	const char *utf8_path;
	NSSavePanel *panel;
	NSOpenPanel *open_panel;
	NSMutableArray *types_array;
	NSURL *default_url;
	char buf[256], *patterns;

	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	if (flags & NOC_FILE_DIALOG_OPEN) {
		panel = open_panel = [NSOpenPanel openPanel];
	}
	else {
		panel = [NSSavePanel savePanel];
	}

	if (flags & NOC_FILE_DIALOG_DIR) {
		[open_panel setCanChooseDirectories : YES];
		[open_panel setCanChooseFiles : NO];
	}

	if (default_path) {
		default_url = [NSURL fileURLWithPath :
		[NSString stringWithUTF8String : default_path]];
		[panel setDirectoryURL : default_url];
		[panel setNameFieldStringValue : default_url.lastPathComponent];
	}

	if (filters) {
		types_array = [NSMutableArray array];
		while (*filters) {
			filters += strlen(filters) + 1; // skip the name
			strcpy(buf, filters);
			buf[strlen(buf) + 1] = '\0';
			for (patterns = buf; *patterns; patterns++)
				if (*patterns == ';') *patterns = '\0';
			patterns = buf;
			while (*patterns) {
				assert(strncmp(patterns, "*.", 2) == 0);
				patterns += 2; // Skip the "*."
				[types_array addObject : [NSString stringWithUTF8String : patterns]];
				patterns += strlen(patterns) + 1;
			}
			filters += strlen(filters) + 1;
		}
		[panel setAllowedFileTypes : types_array];
	}

	if (g_noc_file_dialog_ret != NULL)
		free(g_noc_file_dialog_ret);

	g_noc_file_dialog_ret = NULL;
	if ([panel runModal] == NSModalResponseOK) {
		url = [panel URL];
		utf8_path = [[url path] UTF8String];
		g_noc_file_dialog_ret = strdup(utf8_path);
	}

	[pool release];
	return g_noc_file_dialog_ret;
}

#else
//Linux.
#include <gtk/gtk.h>

static char selected_char[1024];


const char *noc_file_dialog_open(int flags,
	const char *filters,
	const char *default_path,
	const char *default_name)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	GtkFileChooser *chooser;
	GtkFileChooserAction action;
	gint res;
	char buf[128], *patterns;

	if (flags & NOC_FILE_DIALOG_DIR)
	{
		strcpy(selected_char, "");
		FILE *f = popen("zenity --file-selection --directory", "r");
		fgets(selected_char, 1024, f);
		pclose(f);
		int length = strlen(selected_char);
		if (length < 2)
		{
			return NULL;
		}
		if (selected_char[length - 1] == '\n' || selected_char[length - 1] == '\r') selected_char[length - 1] = 0;

		return &selected_char[0];
	}

	if (flags & NOC_FILE_DIALOG_OPEN) {
		char cmd[1024], cmd1[1024];
		//
		if (default_path) {
			if (default_name)
				sprintf(cmd, "zenity --title \"Open File\" --file-selection --filename=\"%s/%s\"", default_path, default_name);
			else
				sprintf(cmd, "zenity --title \"Open File\" --file-selection --filename=\"%s\"", default_path);
		}
		else {
			strcpy(cmd, "zenity --title \"Open File\" --file-selection");
		}


		if (filters) {
			sprintf(cmd1, " --file-filter='(%s) | *.%s'", filters, filters);
			strcat(cmd, cmd1);
		}

		strcpy(selected_char, "");
		FILE *f = popen(cmd, "r");
		fgets(selected_char, 1024, f);
		pclose(f);
		int length = strlen(selected_char);
		if (length < 2)
		{
			return NULL;
		}
		if (selected_char[length - 1] == '\n' || selected_char[length - 1] == '\r') selected_char[length - 1] = 0;

		return &selected_char[0];
	}


	if (flags & NOC_FILE_DIALOG_SAVE) {
		char cmd[1024], cmd1[1024];
		//
		if (default_path) {
			if (default_name)
				sprintf(cmd, "zenity --save --title \"Save File\" --file-selection --filename=\"%s/%s\"", default_path, default_name);
			else {
				if (default_path[strlen(default_path) - 1] == '/')
					sprintf(cmd, "zenity --save --title \"Save File\" --file-selection --filename=\"%s\"", default_path);
				else
					sprintf(cmd, "zenity --save --title \"Save File\" --file-selection --filename=\"%s/\"", default_path);
			}
		}
		else {
			strcpy(cmd, "zenity --save --title \"Save File\" --file-selection");
		}


		if (filters) {
			sprintf(cmd1, " --file-filter='(%s) | *.%s'", filters, filters);
			strcat(cmd, cmd1);
		}

		strcpy(selected_char, "");
		FILE *f = popen(cmd, "r");
		fgets(selected_char, 1024, f);
		pclose(f);
		int length = strlen(selected_char);
		if (length < 2)
		{
			return NULL;
		}
		if (selected_char[length - 1] == '\n' || selected_char[length - 1] == '\r') selected_char[length - 1] = 0;

		return &selected_char[0];
	}

	//zenity --file-selection --file-filter='PDF files (pdf) | *.pdf'
#ifdef USEGTKDIRECTLY
	action = flags & NOC_FILE_DIALOG_SAVE ? GTK_FILE_CHOOSER_ACTION_SAVE :
		GTK_FILE_CHOOSER_ACTION_OPEN;
	if (flags & NOC_FILE_DIALOG_DIR)
		action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

	gtk_init_check(NULL, NULL);
	dialog = gtk_file_chooser_dialog_new(
		flags & NOC_FILE_DIALOG_SAVE ? "Save File" : "Open File",
		NULL,
		action,
		"_Cancel", GTK_RESPONSE_CANCEL,
		flags & NOC_FILE_DIALOG_SAVE ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
		NULL);
	chooser = GTK_FILE_CHOOSER(dialog);
	if (flags & NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION)
		gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

	if (default_path)
		gtk_file_chooser_set_filename(chooser, default_path);
	if (default_name)
		gtk_file_chooser_set_current_name(chooser, default_name);

	while (filters && *filters) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, filters);
		filters += strlen(filters) + 1;

		// Split the filter pattern with ';'.
		strcpy(buf, filters);
		buf[strlen(buf)] = '\0';
		for (patterns = buf; *patterns; patterns++)
			if (*patterns == ';') *patterns = '\0';
		patterns = buf;
		while (*patterns) {
			gtk_file_filter_add_pattern(filter, patterns);
			patterns += strlen(patterns) + 1;
		}

		gtk_file_chooser_add_filter(chooser, filter);
		filters += strlen(filters) + 1;
	}

	res = gtk_dialog_run(GTK_DIALOG(dialog));

	free(g_noc_file_dialog_ret);
	g_noc_file_dialog_ret = NULL;

	if (res == GTK_RESPONSE_ACCEPT)
		g_noc_file_dialog_ret = gtk_file_chooser_get_filename(chooser);
	gtk_widget_destroy(dialog);
	while (gtk_events_pending()) gtk_main_iteration();
	return g_noc_file_dialog_ret;
#endif

}

#endif
#endif


bool CancelQuit()
{
	boxer::Selection selection;
	selection = boxer::show("Are you sure you want to quit ?", " Warning!", boxer::Style::Question, boxer::Buttons::OKCancel);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;


	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Cancel) {
		return true;
	}

	return false;
}

bool overWriteFileBox(char * file)
{
	boxer::Selection selection;
	selection = boxer::show(" File exists, do you want to overwrite file?", " Warning!", boxer::Style::Question, boxer::Buttons::YesNo);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;
	io.KeysDown[13] = false;
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	if (selection == boxer::Selection::Yes) return(true);

	return(false);
}


int askBoxCancel(const char * ask, const char *title)
{
	boxer::Selection selection;
	selection = boxer::show(ask, title, boxer::Style::Question, boxer::Buttons::YesNoCancel);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false; //also reset imgui keys.
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Yes) return(1);
	if (selection == boxer::Selection::Cancel) return(2);
	return(0);
}

bool askBox(char * ask, char *title)
{
	boxer::Selection selection;
	selection = boxer::show(ask, title, boxer::Style::Question, boxer::Buttons::YesNo);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false; //also reset imgui keys.
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Yes) return(true);

	return(false);
}

bool changedFileBox(char * file)
{
	boxer::Selection selection;
	char msg[1024];

	sprintf(msg, "The file has unsaved changes. Do you want to save it?\n%s", file);
	selection = boxer::show(msg, " Warning!", boxer::Style::Question, boxer::Buttons::YesNo);

	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false;
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;

	if (selection == boxer::Selection::Yes) return(true);

	return(false);
}

void BoxerInfo(char * text, const char *heading)
{
	boxer::show(text, heading);
	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;

	io.KeysDown[13] = false;
	io.KeysDown[16] = false;
	io.KeysDown[17] = false;
	io.KeysDown[18] = false;
	io.KeysDown[19] = false;
	//PE: Need to reset any system wide hotkey that execute a blocking dialog.
	io.KeysDown[0x7B] = false; //F12
	io.KeysDown[78] = false;
	io.KeysDown[79] = false;
	io.KeysDown[83] = false;
	io.KeysDown[90] = false;
	io.KeysDown[73] = false;
	io.KeysDown[69] = false; //New CTRL-Keys added. E-N-L-SPACE
	io.KeysDown[78] = false;
	io.KeysDown[76] = false;
	io.KeysDown[32] = false;

	io.MouseDown[0] = 0;
	io.MouseDown[1] = 0;
	io.MouseDown[2] = 0;
	io.MouseDown[3] = 0;


}

void DebugInfo(char * text, const char *heading)
{
#ifndef DEVVERSION
	return;
#else
	boxer::show(text, heading);
	//Make sure ther blocking dialog did not skip some keys, reset.
	ImGuiIO& io = ImGui::GetIO();
	io.KeySuper = false;
	io.KeyCtrl = false;
	io.KeyAlt = false;
	io.KeyShift = false;
#endif
}



