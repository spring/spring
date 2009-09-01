--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    VFSUtils.lua
--  brief:   utility function for VFS access
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function RecursiveFileSearch(startingDir, fileType, vfsMode)
  local files = {}
  local function AddSubDir(dir)
    for _,file in ipairs(VFS.DirList(dir, fileType, vfsMode)) do
      files[#files + 1] = file
    end
    for _,sd in ipairs(VFS.SubDirs(dir, "*", vfsMode)) do
      AddSubDir(sd)
    end
  end

  AddSubDir(startingDir)

  return files
end
