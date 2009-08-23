--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    VFSUtils.lua
--  brief:   utility function for VFS access
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function RecursiveFileSearch(startingDir, fileType)
  local files = {}
  local function AddSubDir(dir)
    for _,file in ipairs(VFS.DirList(dir, fileType)) do
      files[#files + 1] = file
    end
    for _,sd in ipairs(VFS.SubDirs(dir)) do
      AddSubDir(sd)
    end
  end
  
  AddSubDir(startingDir)
  
  return files
end 
