function getPathDirectory( filename )  
    res = string.match(filename, "(.+)[\\|^/][^\\|^/]*%.%w+$")
	if res == nil then
		res = "."
	end
	return res
end 

function getPathFileName( filename )  
    res = string.match(filename, ".+[\\|^/]([^\\|^/]*%.%w+)$")
	return res
end 

function getFileExtention( filename )
	return string.match(filename, ".+%.(%w+)$")
end

function getSubDirectories( directory )
   local i, t, popen = 0, {}, io.popen
   
   --for filename in popen('ls -a Q*.lua "'..directory..'"'):lines() do  --Linux 
   for filename in popen('dir "'..directory..'" /b /ad'):lines() do 
      i = i + 1
      t[i] = filename
   end
   return t
end

function getDirectoryFiles( directory )
   local i, t, popen = 0, {}, io.popen
   
   --for filename in popen('ls -a Q*.lua "'..directory..'"'):lines() do  --Linux 
   for filename in popen('dir "'..directory..'" /b /a-d'):lines() do 
      i = i + 1
      t[i] = filename
   end
   return t
end

function fileExists( filename )    
	local f = io.open(filename)    
	if f == nil then        
		return false    
	end    
	io.close(f)    
	return true
end

function folderExists( strFolderName )
	local fileHandle, strError = io.open(strFolderName.."\\*.*","r")
	if fileHandle ~= nil then
		io.close(fileHandle)
		return true
	else
		if string.match(strError,"No such file or directory") then
			return false
		else
			return true
		end
	end
end

function clearAndCreateFolder( folderName )
	if folderExists(folderName) then
		os.execute("del /Q " .. '"' .. folderName .. '"')
	else
		os.execute("mkdir " .. '"' .. folderName .. '"')
	end
end

function deleteFolder( folderName )
	if folderExists(folderName) then
		os.execute("del /Q " .. '"' .. folderName .. '"')
		os.execute("rd /Q " .. '"' .. folderName .. '"')
	end
end

local _currentScriptDir = "."
function GetCurrentDirectory()
	return _currentScriptDir
end

function Run( scriptFile )
	_currentScriptDir = getPathDirectory(scriptFile)
	dofile(scriptFile)
end

function LoadScene( sceneFile )
	if fileExists(sceneFile) then
		return _LoadScene(sceneFile)
	else 
		otherSceneFilePath = _currentScriptDir .. "/" .. sceneFile
		return _LoadScene(otherSceneFilePath)
	end
end
	