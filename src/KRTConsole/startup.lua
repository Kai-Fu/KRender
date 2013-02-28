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

function ConvertFBX( fbxFile, outDir )
	
	out_file = outDir .. '/scene.scn'
	clearAndCreateFolder(outDir)
	cmd = "KFBXTranslator " .. '"' .. fbxFile .. '" "' .. out_file .. '"'
	res = os.execute(cmd)
	if res == -1 then
		return nil, nil
	else
		-- returns the converted file name and the frame count(0 if the FBX is static scene)
		frame_updates = {}
		for frame_i = 0, res do
			local cur_frame_file = out_file .. ".frame" .. string.format("%05d", frame_i)
			table.insert(frame_updates, cur_frame_file)
		end
		return out_file, frame_updates
	end
end

function Run( scriptFile )
	_currentScriptDir = getPathDirectory(scriptFile)
	dofile(scriptFile)
end
	