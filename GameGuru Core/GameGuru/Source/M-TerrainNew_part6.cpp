void PositionCameraForNewLevel()
{
	if (newLevelCamera.set)
	{
		t.editorfreeflight.c.y_f = newLevelCamera.y + 100;
		t.editorfreeflight.c.x_f = newLevelCamera.x;
		t.editorfreeflight.c.z_f = newLevelCamera.z;
		t.editorfreeflight.c.angx_f = t.gridentityrotatex_f + 5.0f;
		t.editorfreeflight.c.angy_f = t.gridentityrotatey_f;
		t.editorfreeflight.s = t.editorfreeflight.c;

		newLevelCamera.set = 0;
	}
}

#ifdef CUSTOMTEXTURES
void ChooseTerrainTextureFolder(char* folder)
{
	cstr oldDir = GetDir();

	char writePath[MAX_PATH];

	extern const char* GG_GetWritePath();
	strcpy(writePath, GG_GetWritePath());
	
	// Determine default path to the writable folder textures
	char writableTextures[MAX_PATH];
	strcpy(writableTextures, writePath);
	strcat(writableTextures, "Files\\terraintextures\\");
	if (PathExist(writableTextures) == 0)
	{
		MakeDirectory(writableTextures);
	}

	char destination[MAX_PATH];
	bool bInMaxFolder = false;
	
	// User chooses new texture folder
	if (folder == nullptr)
	{
		// File dialog to choose texture directory
		char* cNewDirectory = (char *)noc_file_dialog_open(NOC_FILE_DIALOG_DIR, "All\0*.*\0", writableTextures, NULL, true, "Choose a folder to take textures from");
		if (cNewDirectory && strlen(cNewDirectory) > 0)
		{
			// Check if chosen new location for textures is already in Max installation, or writable folder
			bool bInWritableFolder = false;
			if (strstr(cNewDirectory, writableTextures))
			{
				bInWritableFolder = true;
			}
			if (!bInWritableFolder)
			{
				char maxTextures[MAX_PATH];
				strcpy(maxTextures, g.fpscrootdir_s.Get());
				strcat(maxTextures, "\\Files\\");
				if (strstr(cNewDirectory, maxTextures))
				{
					bInMaxFolder = true;
				}
			}

			if (!bInWritableFolder && !bInMaxFolder)
			{
				// Chosen textures are not in Max install or writable, so we need to copy them to the writable folder
				std::vector<std::string> filesToCopy;
				CollectFilesWithExtension(".png", cNewDirectory, &filesToCopy);
				if (filesToCopy.empty())
				{
					extern bool bTriggerMessage;
					extern char cTriggerMessage[MAX_PATH];
					bTriggerMessage = true;
					strcpy(cTriggerMessage, "Could not find any PNG textures");
					ResetTextureSettings();
					return;
				}
				char folder[MAX_PATH];
				for (int i = strlen(cNewDirectory) - 1; i >= 0; i--)
				{
					if (cNewDirectory[i] == '/' || cNewDirectory[i] == '\\')
					{
						strcpy(folder, cNewDirectory + i + 1);
						strcat(folder, "\\");
						break;
					}
				}
				// Create the destination directory and copy the files into the writable folder
				strcpy(destination, writableTextures);
				strcat(destination, folder);
				int sourceDirectoryLength = strlen(cNewDirectory);
				char lastChar = cNewDirectory[sourceDirectoryLength - 1];
				if (lastChar != '/' && lastChar != '\\')
				{
					sourceDirectoryLength += 1;
				}
				if (PathExist(destination) == 0)
				{
					MakeDirectory(destination);
				}
				for (auto& file : filesToCopy)
				{
					// Extract filename only from the source file and copy to new location
					std::string newFilename = destination;
					const char* filePath = file.c_str() + sourceDirectoryLength;
					if (filePath)
					{
						// Check if the file is contained within another folder (will need to create the directory in the writable folder)
						char subfolder[MAX_PATH];
						strcpy(subfolder, filePath);
						bool bIsFolder = false;
						for (int i = strlen(subfolder) - 1; i >= 0; i--)
						{
							if (subfolder[i] == '/' || subfolder[i] == '\\')
							{
								subfolder[i + 1] = 0;
								bIsFolder = true;
								break;
							}
						}
						if (bIsFolder)
						{
							char pathToSubfolder[MAX_PATH];
							strcpy(pathToSubfolder, destination);
							strcat(pathToSubfolder, subfolder);
							if (PathExist(pathToSubfolder) == 0)
							{
								MakeDirectory(pathToSubfolder);
							}
						}
						newFilename.append(filePath);
					}
					// Perform the copy
					CopyAFile((char*)file.c_str(), (char*)newFilename.c_str());
				}
			}
			else
			{
				// Already in max installation or writable, so no need to copy any files
				strcpy(destination, cNewDirectory);
				strcat(destination, "\\");
			}

			SetDir(oldDir.Get());

			// Change textures location to the new folder chosen by the user
			char cMaxPath[MAX_PATH];
			strcpy(cMaxPath, g.fpscrootdir_s.Get());
			strcat(cMaxPath, "\\Files\\");
			char* inMaxPath = strstr(destination, cMaxPath);
			if (inMaxPath)
			{
				t.visuals.customTexturesFolder = destination + strlen(cMaxPath);
			}
			else
			{
				char* inWritablePath = strstr(destination, writePath);
				if (inWritablePath)
				{
					t.visuals.customTexturesFolder = destination + strlen(writePath);
				}
				else
				{
					t.visuals.customTexturesFolder = destination;
				}
			}

			// and as soon as change texture, create the custommaterials file so level has this when it is saved
			cstr terrainMaterialFile = g.mysystem.levelBankTestMap_s + "custommaterials.dat";
			SaveTerrainTextureFolder(terrainMaterialFile.Get());
		}
		else
		{
			return;
		}
	}
	else
	{
		// Determine full path to custom textures folder (writable or max install)
		// so that we can load the textures during the refresh below (destination needs to point to valid area!!)
		if (strlen(folder) > 0)
		{
			char fullPath[MAX_PATH];
			strcpy(fullPath, writePath);
			strcat(fullPath, "\\Files\\");
			strcat(fullPath, folder);
			if (PathExist(fullPath) == 0)
			{
				fullPath[0] = 0;
				strcpy(fullPath, g.fpscrootdir_s.Get());
				strcat(fullPath, "\\Files\\");
				strcat(fullPath, folder);
				if (PathExist(fullPath) == 0)
				{
					// for now, no warning, just silent fail
					return;
				}
			}

			// Loading texture folder
			t.visuals.customTexturesFolder = folder;
			strcpy(destination, fullPath);
		}
		else
		{
			// passed in empty string, meaning we want to force a reset to stock terrain textures (used by Terrain Generator Biome selector)
			ResetTextureSettings();
			return;
		}

		// as loading direct, always in MAX folder
		bInMaxFolder = true;
	}

	std::vector<std::string> files;
	CollectFilesWithExtension("color.dds", destination, &files);
	bool bNeedToConvertPNGs = false;
	if (files.empty())
	{
		CollectFilesWithExtension("color.png", destination, &files);
		bNeedToConvertPNGs = true;
	}

	char newDir[MAX_PATH];
	strcpy(newDir, destination);
	if (bInMaxFolder)
	{
		newDir[strlen(newDir) - strlen(t.visuals.customTexturesFolder.Get())] = 0;
	}
	else
	{
		newDir[strlen(newDir) - strlen(t.visuals.customTexturesFolder.Get()) + strlen("Files\\")] = 0;
	}

	char* localPath = strstr(destination, "Files\\");
	if (localPath)
	{
		int pathOffset = strlen(destination) - strlen(localPath) + strlen("Files\\");
		for (int i = 0; i < 32; i++)
		{
			// Delete existing image
			sTerrainTexturesID[i] = t.terrain.imagestartindex + 80 + i;
			if (ImageExist(t.terrain.imagestartindex + 80 + i))
			{
				DeleteImage(t.terrain.imagestartindex + 80 + i);
			}

			// Set new custom terrain textures
			if (files.size() > i)
			{
				t.visuals.sTerrainTextures[i] = files[i].c_str() + pathOffset;
				sTerrainSelectionID[i] = i;
			}
			else
			{
				if (i == files.size())
				{
					t.visuals.sTerrainTextures[i] = g.fpscrootdir_s.Get();
					strcat(t.visuals.sTerrainTextures[i].Get(), "\\Files\\");
					strcat(t.visuals.sTerrainTextures[i].Get(), "terraintextures\\mat32\\Color.dds");
					sTerrainSelectionID[i] = i;
				}
				else
				{
					t.visuals.sTerrainTextures[i] = "";
				}
			}
			t.gamevisuals.sTerrainTextures[i] = t.visuals.sTerrainTextures[i];
		}
	}

	// new path for custom terrain textures
	SetDir(newDir);
	g_DeferTextureUpdateCurrentFolder_s = newDir;

	// For now, load any color textures
	g_DeferTextureUpdate.clear();
	g_DeferTextureUpdate.reserve(32);
	for (int i = 0; i < 32; i++)
	{
		std::string texture = t.visuals.sTerrainTextures[i].Get();
		g_DeferTextureUpdate.push_back(texture);
	}
	if (bNeedToConvertPNGs)
	{
		TerrainConvertPNGFiles(&g_DeferTextureUpdate);
	}

	// Ensure terrain textures are stored as dds
	for (int i = 0; i < 32; i++)
	{
		char textureName[MAX_PATH];
		strcpy(textureName, t.visuals.sTerrainTextures[i].Get());
		if (strcmp(textureName + strlen(textureName) - 4, ".png") == 0)
		{
			textureName[strlen(textureName) - 4] = 0;
			strcat(textureName, ".dds");
			t.visuals.sTerrainTextures[i] = textureName;
		}
		strcpy(textureName, g_DeferTextureUpdate[i].c_str());
		if (strcmp(textureName + strlen(textureName) - 4, ".png") == 0)
		{
			textureName[strlen(textureName) - 4] = 0;
			strcat(textureName, ".dds");
			g_DeferTextureUpdate[i] = textureName;
		}
	}
	
	// trigger an update of the terrain textures when the time is right
	g_iDeferTextureUpdateToNow = 2;

	// restore before leave
	SetDir(oldDir.Get());
}

#include "Nlohmann JSON/json.hpp"
void SaveTerrainTextureFolder(LPSTR pFile)
{
	nlohmann::json j;
	j["TexturesFolder"] = t.visuals.customTexturesFolder.Get();
	j["MaterialSounds"] = g_iCustomTerrainMatSounds;
	std::string content = j.dump(4);

	if (FileExist(pFile) == 1)
	{
		DeleteAFile(pFile);
	}
	if (FileOpen(1) == 1) 
	{ 
		CloseFile(1); 
	}

	std::ofstream outputStream;
	outputStream.open(pFile);
	if (outputStream.is_open())
	{
		outputStream << content;
		outputStream.close();
	}
}

void LoadTerrainTextureFolder(LPSTR pFile)
{
	// if file exists, load it
	if (FileExist(pFile) == 1)
	{
		std::ifstream inputStream;
		inputStream.open(pFile);
		if (inputStream.is_open())
		{
			nlohmann::json j;
			inputStream >> j;
			inputStream.close();
			if (j.contains("TexturesFolder"))
			{
				std::string folder = j.at("TexturesFolder");
				t.visuals.customTexturesFolder = folder.c_str();
				if (folder.length() > 0)
				{
					// trigger refresh of terrain textures from custom texture folder
					ChooseTerrainTextureFolder(t.visuals.customTexturesFolder.Get());
				}
				else
				{
					// Default choice of textures
					g_iDeferTextureUpdateToNow = 1;
				}
			}
			if (j.contains("MaterialSounds"))
			{
				j.at("MaterialSounds").get_to(g_iCustomTerrainMatSounds);
			}
		}
	}
}

#include "PNGToDDSTypes.h"
void TerrainConvertPNGFiles(std::vector<std::string>* colorFiles)
{
	extern int ConvertMaterialToDDSFromPNG(MaterialToConvert*);
	if (colorFiles)
	{
		for (auto& file : *colorFiles)
		{
			if (file.length() > 4)
			{
				char texName[MAX_PATH];
				strcpy(texName, file.c_str());
				MaterialToConvert mat;
				mat.color[0] = file;
				texName[strlen(texName) - 4] = 0;
				strcat(texName, ".dds");
				mat.color[1] = texName;
				texName[strlen(texName) - strlen("color.dds")] = 0;
				strcat(texName, "normal.png");
				mat.normal[0] = texName;
				texName[strlen(texName) - 4] = 0;
				strcat(texName, ".dds");
				mat.normal[1] = texName;
				texName[strlen(texName) - strlen("normal.dds")] = 0;
				strcat(texName, "ao.png");
				mat.ao = texName;
				texName[strlen(texName) - strlen("ao.png")] = 0;
				strcat(texName, "roughness.png");
				mat.roughness = texName;
				texName[strlen(texName) - strlen("roughness.png")] = 0;
				strcat(texName, "metalness.png");
				mat.metalness = texName;
				texName[strlen(texName) - strlen("metalness.png")] = 0;
				strcat(texName, "surface.png");
				mat.surface[0] = texName;
				texName[strlen(texName) - 4] = 0;
				strcat(texName, ".dds");
				mat.surface[1] = texName;
				ConvertMaterialToDDSFromPNG(&mat);
			}
		}
	}
}

void ResetTextureSettings()
{
	t.visuals.customTexturesFolder = "";
	for (int i = 0; i < 32; i++)
	{
		// Delete existing images
		sTerrainTexturesID[i] = t.terrain.imagestartindex + 80 + i;
		if (ImageExist(t.terrain.imagestartindex + 80 + i))
		{
			DeleteImage(t.terrain.imagestartindex + 80 + i);
		}
		t.visuals.sTerrainTextures[i] = "";
	}
	g_iDeferTextureUpdateToNow = 1;

	// Trigger update of material sounds
	extern bool g_bMapMatIDToMatIndexAvailable;
	g_bMapMatIDToMatIndexAvailable = false;
}
#endif



