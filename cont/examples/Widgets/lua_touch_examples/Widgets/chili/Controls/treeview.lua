--//=============================================================================

TreeView = Control:Inherit{
  classname = "treeview",

  autosize = true,

  minItemHeight = 16,

  defaultWidth  = "100%",
  defaultHeight = "100%",

  selected = 1,
  root = nil,
  nodes = {},

  defaultExpanded = false,

  OnSelectNode = {},
}

local this = TreeView
local inherited = this.inherited

--//=============================================================================

local function ParseInitTable(node, nodes)
  local lastnode = node
  for i=1,#nodes do
    local data = nodes[i]
    if (type(data)=="table") then
      ParseInitTable(lastnode, data)
    else
      lastnode = node:Add(data)
    end
  end
end


function TreeView:New(obj)
  local nodes = obj.nodes
  if (nodes) then
    obj.children = {}
  end

  obj = inherited.New(self,obj)

  obj.root = TreeViewNode:New{treeview = obj, root = true; minHeight = obj.minItemHeight; expanded = self.defaultExpanded}
  if (nodes) then
    ParseInitTable(obj.root, nodes)
  end
  obj:AddChild(obj.root)

  obj:UpdateLayout()

  local sel = obj.selected
  obj.selected = false
  if ((sel or 0)>0) then
    obj:Select(sel)
  end

  return obj
end

--//=============================================================================

function TreeView:GetNodeByCaption(caption, _node)
  if (not _node) then
    return self:GetNodeByCaption(caption, self.root)
  end

  for i=1,#_node.nodes do
    local n = _node.nodes[i]
    if (n.caption == caption) then
      return n
    end

    local result = self:GetNodeByCaption(caption, n, _i)
    if (result) then
      return result
    end
  end
end


function TreeView:GetNodeByIndex(index, _node, _i)
  if (not _node) then
    local result = self:GetNodeByIndex(index, self.root, 0) --// 0, cause we don't count the root node! (-> it really starts with 1!)
    return (not IsNumber(result)) and result
  end

  for i=1,#_node.nodes do
    _i = _i + 1
    if (_i == index) then
      return _node.nodes[i]
    end

    local result = self:GetNodeByIndex(index, _node.nodes[i], _i)
    if (IsNumber(result)) then
      _i = result
    else
      return result
    end
  end

  return _i
end

--//=============================================================================

function TreeView:Select(item)
  local obj = UnlinkSafe(item)

  if (type(item) == "number") then
    obj = self:GetNodeByIndex(item)
  end

  if (obj and obj:InheritsFrom("treeviewnode")) then
    local oldSelected = self.selected
    self.selected = MakeWeakLink(obj)
    self.selected:Invalidate()
    if (oldSelected) then
      oldSelected:Invalidate()
    end

    obj:CallListeners(obj.OnSelectChange, true)
    if (oldSelected) then oldSelected:CallListeners(oldSelected.OnSelectChange, false) end
    self:CallListeners(self.OnSelectNode, self.selected, oldSelected)
  end
end

--//=============================================================================

function TreeView:UpdateLayout()
  local c = self.root
  c:_UpdateConstraints(0, 0, self.clientWidth)
  c:Realign()

  if (self.autosize) then
    self:Resize(nil, c.height, true, true)
  end

  return true
end

--//=============================================================================