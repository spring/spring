--//=============================================================================

TreeViewNode = Control:Inherit{
  classname = "treeviewnode",

  padding = {16,0,0,0},

  autosize = true,
  caption   = "node",
  expanded  = true,

  root      = false,
  nodes     = {},

  treeview  = nil,

  OnSelectChange = {},
  OnCollapse     = {},
  OnExpand       = {},
  OnDraw         = {},
}

local this = TreeViewNode
local inherited = this.inherited

--//=============================================================================

function TreeViewNode:New(obj)
  if (obj.root) then
    obj.padding = {0,0,0,0}
  end

  obj = inherited.New(self,obj)
  return obj
end


function TreeViewNode:AddChild(obj, isNode)
  if (isNode~=false) then
    self.nodes[#self.nodes+1] = obj
  end
  return inherited.AddChild(self,obj)
end


function TreeViewNode:RemoveChild(obj)
  local result = inherited.RemoveChild(self,obj)

  local nodes = self.nodes
  for i=1,#nodes do
    if CompareLinks(nodes[i],obj) then
      table.remove(nodes, i)
      return result
    end
  end

  return result
end


--//=============================================================================

function TreeViewNode:Add(item)
  local newnode
  if (type(item) == "string") then
    local lbl = TextBox:New{text = item; width = "100%"; padding = {2,3,2,2}; minHeight = self.minHeight;}
    newnode = TreeViewNode:New{caption = item; treeview = self.treeview; minHeight = self.minHeight; expanded = self.expanded;}
    newnode:AddChild(lbl, false)
    self:AddChild(newnode)
  elseif (IsObject(item)) then
    newnode = TreeViewNode:New{caption = ""; treeview = self.treeview; minHeight = self.minHeight; expanded = self.expanded;}
    newnode:AddChild(item, false)
    self:AddChild(newnode)
  end
  return newnode
end


function TreeViewNode:Select()
  if (self.root) then
    return
  end

  if (not CompareLinks(self.treeview.selected,self)) then
    --// treeview itself calls node:OnSelectChange !
    self.treeview:Select(self)
  end
end


function TreeViewNode:Toggle()
  if (self.root) then
    return
  end

  if (self.expanded) then
    self:Collapse()
  else
    self:Expand()
  end
end


function TreeViewNode:Expand()
  if (self.root) then
    return
  end

  self:CallListeners(self.OnExpand)
  self.expanded = true
  self.treeview:RequestRealign()
end


function TreeViewNode:Collapse()
  if (self.root) then
    return
  end

  self:CallListeners(self.OnCollapse)
  self.expanded = false
  self.treeview:RequestRealign()
end

--//=============================================================================

function TreeViewNode:UpdateLayout()
  local clientWidth = self.clientWidth
  local children = self.children

  if (not self.expanded)and(children[1])and(not self.root) then
    local c = children[1]
    c:_UpdateConstraints(0, 0, clientWidth)
    c:Realign()
    self:Resize(nil, c.height, true, true)

    --//FIXME this still calls UpdateLayout -> slow
    --//    better override CallChildrenXYZ!

    if (children[2])and(children[2].x < 1e9) then
      --// hide the subnodes in nirvana
      for i=2,#children do
        c = children[i]
        c:_UpdateConstraints(1e9, 1e9)
        c:Realign()
      end
    end

    return true
  end


  local y = 0
  for i=1,#children do
    local c = children[i]
    c:_UpdateConstraints(0, y, clientWidth)
    c:Realign()
    y = y + c.height
  end

  self:Resize(nil, y, true, true)
  return true
end

--//=============================================================================

function TreeViewNode:_InNodeButton(x,y)
  if (self.root) then
    return false
  end

  if (x>=self.padding[1]) then
    return false
  end

  local nodeTop = (self.children[1].height - self.padding[1])*0.5
  return (nodeTop<=y)and(y-nodeTop<self.padding[1])
end


function TreeViewNode:HitTest(x,y, ...)
  local obj = inherited.HitTest(self,x,y, ...)
  if (obj) then return obj end
  --if (self:_InNodeButton(x,y)) then
    return self
  --end
end


function TreeViewNode:MouseDown(x,y, ...)
  if (self.root) then
    return inherited.MouseDown(self, x, y, ...)
  end

  --//FIXME this function is needed to recv MouseClick -> fail
  if (self:_InNodeButton(x,y)) then
    return self
  end

  if (x>=self.padding[1])then
    --[[ FIXME inherited.MouseDown should be executed before Select()!
    local obj = inherited.MouseDown(self, x, y, ...)
    return obj
    --]]

    if (y < self.padding[2]+self.children[1].height) then
      self:Select()
    end

    local obj = inherited.MouseDown(self, x, y, ...)
    return obj
  end
end


function TreeViewNode:MouseClick(x,y, ...)
  if (self.root) then
    return inherited.MouseClick(self, x, y, ...)
  end

  if (self:_InNodeButton(x,y)) then
    self:Toggle()
    return self
  end

  local obj = inherited.MouseClick(self, x, y, ...)
  return obj
end


function TreeViewNode:MouseDblClick(x,y, ...)
--//FIXME doesn't get called, related to the FIXME above!
  if (self.root) then
    return inherited.MouseDblClick(self, x, y, ...)
  end

  local obj = inherited.MouseDblClick(self, x, y, ...)
  if (not obj) then
    self:Toggle()
    obj = self
  end

  return obj
end

--//=============================================================================

function TreeViewNode:DrawNode()
  self.treeview.DrawNode(self)
end


function TreeViewNode:DrawNodeTree()
  self.treeview.DrawNodeTree(self)
end


function TreeViewNode:DrawControl()
  if (self.root) then
    return
  end

  local dontDraw = self:CallListeners(self.OnDraw, self)
  if (not dontDraw) then
    self:DrawNode()
  end
  self:DrawNodeTree()
end


function TreeViewNode:DrawChildren()
  if not (self.expanded or self.root) then
    self:_DrawInClientArea(self.children[1].Draw,self.children[1])
    return
  end

  if (next(self.children)) then
    self:_DrawChildrenInClientArea('Draw')
  end
end


function TreeViewNode:DrawChildrenForList()
  if not (self.expanded or self.root) then
    self:_DrawInClientArea(self.children[1].DrawForList,self.children[1])
    return
  end

  if (next(self.children)) then
    self:_DrawChildrenInClientArea('DrawForList')
  end
end

--//=============================================================================