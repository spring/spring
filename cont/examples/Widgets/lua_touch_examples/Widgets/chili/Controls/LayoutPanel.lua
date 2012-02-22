--//=============================================================================
--// This control is the base of any auto-layout panel, this includes:
--//   grid, stackpanel, tables, itemlistview, listbox, radiogroups, ...
--//
--// Internal all childrens/items are handled via cells, which can be
--// freely aligned (table-grid, free movable like in imagelistviews, ...).
--//
--// Also most subclasses should use an items table to create their child
--// objects, so the user just define the captions, filenames, ... instead
--// of creating checkboxes, images, ... directly.
--// This doesn't affect simple containers like grids & stackcontrols, which
--// don't create any controls themselves (instead they just align their children).

LayoutPanel = Control:Inherit{
  classname = "layoutpanel",

  itemMargin    = {5, 5, 5, 5},
  itemPadding   = {5, 5, 5, 5},
  minItemWidth  = 0,
  maxItemWidth  = 0,
  minItemHeight = 0,
  maxItemHeight = 0,

  autosize = false,

  rows = nil,
  columns = nil,
  orientation   = "horizontal", --// "horizontal" or "vertical"
  autoArrangeH  = false, --FIXME rename
  autoArrangeV  = false, --FIXME rename
  grid          = false, --FIXME not implemented yet // if true, each row should have the same number of columns (table layout)
  resizeItems   = false,
  centerItems   = true,

  --[[ takes weights into account when resizing items (instead of same size for all)
       - e.g. "component1.weight = 1, component2.weight = 2" => component2 will be 2 times larger than component1
       - if all components have same weight -> same layout as without weightedResize
       - default value is 1 (nil interpreted as 1)
       - with manageWeights the weight is automatically set depending on number of children
  ]]
  weightedResize = false,

  selectable    = false,
  multiSelect   = true,
  selectedItems = {},

  OnSelectItem = {},
  OnDrawItem = {}, --FIXME
  OnDblClickItem = {},

  _rows = nil,
  _columns = nil,
  _cells = nil,
}

local this = LayoutPanel
local inherited = this.inherited

--//=============================================================================

function LayoutPanel:New(obj)
  obj = inherited.New(self,obj)
  if (obj.selectable) then
    obj.selectedItems = {[1]=true}
  end
  return obj
end

--//=============================================================================

function LayoutPanel:SetOrientation(orientation)
  self.orientation = orientation
  inherited.UpdateClientArea(self)
end

--//=============================================================================

local tsort = table.sort

--//=============================================================================

local function compareSizes(a,b)
  return a[2] < b[2]
end

function LayoutPanel:_JustCenterItemsH(startCell,endCell,freeSpace)
  local _cells = self._cells
  local perItemHalfAlloc = (freeSpace/2) / ((endCell+1) - startCell)
  for i=startCell,endCell do
    local cell = _cells[i]
    --if (self.orientation == "horizontal") then
      cell[1] = cell[1] + perItemHalfAlloc
    --else
    --  cell[2] = cell[2] + perItemHalfAlloc
    --end
  end
end

function LayoutPanel:_JustCenterItemsV(startCell,endCell,freeSpace)
--[[
  local _cells = self._cells
  --local startCell = 1
  --local endCell = #_cells

  local perItemHalfAlloc = (freeSpace/2) / ((endCell+1) - startCell)
  for i=startCell,endCell do
    local cell = _cells[i]
    --if (self.orientation == "horizontal") then
      cell[2] = cell[2] + perItemHalfAlloc
    --else
    --  cell[1] = cell[1] + perItemHalfAlloc
    --end
  end
--]]
end


function LayoutPanel:_EnlargeToLineHeight(startCell, endCell, lineHeight)
  local _cells = self._cells

  for i=startCell,endCell do
    local cell = _cells[i]
    --if (self.orientation == "horizontal") then
      cell[4] = lineHeight
    --else
    --  cell[3] = lineHeight
    --end
  end
end


function LayoutPanel:_AutoArrangeAbscissa(startCell,endCell,freeSpace)
  if (startCell > endCell) then
    return
  end

  if (not self.autoArrangeH) then
    if (self.centerItems) then
      self:_JustCenterItemsH(startCell,endCell,freeSpace)
    end
    return
  end

  local _cells = self._cells

  if (startCell == endCell) then
    local cell = self._cells[startCell]
    if (self.orientation == "horizontal") then
      cell[1] = cell[1] + freeSpace/2
    else
      cell[2] = cell[2] + freeSpace/2
    end
    return
  end

  --// create a sorted table with the cell sizes
  local cellSizesCount = 0
  local cellSizes = {}
  for i=startCell,endCell do
    cellSizesCount = cellSizesCount + 1
    if (self.orientation == "horizontal") then
      cellSizes[cellSizesCount] = {i,_cells[i][3]}
    else
      cellSizes[cellSizesCount] = {i,_cells[i][4]}
    end
  end
  tsort(cellSizes,compareSizes)

  --// upto this index all cells have the same size (in the cellSizes table)
  local sameSizeIdx = 1
  local shortestCellSize = cellSizes[1][2]

  while (freeSpace>0)and(sameSizeIdx<cellSizesCount) do

    --// detect the cells, which have the same size
    for i=sameSizeIdx+1,cellSizesCount do
      if (cellSizes[i][2] ~= shortestCellSize) then
        break
      end
      sameSizeIdx = sameSizeIdx + 1
    end

    --// detect 2. shortest cellsize
    local nextCellSize = 0
    if (sameSizeIdx >= cellSizesCount) then
      nextCellSize = self.maxItemWidth --FIXME orientation
    else
      nextCellSize = cellSizes[sameSizeIdx+1][2]
    end


    --// try to fillup the shorest cells to the 2. shortest cellsize (so we can repeat the process on n+1 cells)
    --// (if all/multiple cells have the same size share the freespace between them)
    local spaceToAlloc = shortestCellSize - nextCellSize
    if (spaceToAlloc > freeSpace) then
      spaceToAlloc = freeSpace
      freeSpace    = 0
    else
      freeSpace    = freeSpace - spaceToAlloc
    end
    local perItemAlloc = (spaceToAlloc / sameSizeIdx)


    --// set the cellsizes/share the free space between cells
    for i=1,sameSizeIdx do
      local celli = cellSizes[i]
      celli[2] = celli[2] + perItemAlloc --FIXME orientation

      local cell = _cells[ celli[1] ]
      cell[3] = cell[3] + perItemAlloc --FIXME orientation

      --// adjust the top/left startcoord of all following cells (in the line)
      for j=celli[1]+1,endCell do
        local cell = _cells[j]
        cell[1] = cell[1] + perItemAlloc --FIXME orientation
      end
    end

    shortestCellSize = nextCellSize

  end
end


function LayoutPanel:_AutoArrangeOrdinate(freeSpace)
  if (not self.autoArrangeV) then
--[[
    if (self.centerItems) then
      local startCell = 1
      local endCell = 1
      for i=2,#self._lines do
        endCell = self._lines[i] - 1
        self:_JustCenterItemsV(startCell,endCell,freeSpace)
        startCell = endCell + 1
      end
    end
--]]
    return
  end

  local _lines = self._lines
  local _cells = self._cells

  --// create a sorted table with the line sizes
  local lineSizes = {}
  for i=1,#_lines do
    local first_cell_in_line = _cells[ _lines[i] ]
    if (self.orientation == "horizontal") then --FIXME
      lineSizes[i] = {i,first_cell_in_line[4]}
    else
      lineSizes[i] = {i,first_cell_in_line[3]}
    end
  end
  tsort(lineSizes,compareSizes)
  local lineSizesCount = #lineSizes

  --// upto this index all cells have the same size (in the cellSizes table)
  local sameSizeIdx = 1
  local shortestLineSize = lineSizes[1][2]

  while (freeSpace>0)and(sameSizeIdx<lineSizesCount) do

    --// detect the lines, which have the same size
    for i=sameSizeIdx+1,lineSizesCount do
      if (lineSizes[i][2] ~= shortestLineSize) then
        break
      end
      sameSizeIdx = sameSizeIdx + 1
    end

    --// detect 2. shortest linesize
    local nextLineSize = 0
    if (sameSizeIdx >= lineSizesCount) then
      nextLineSize = self.maxItemHeight --FIXME orientation
    else
      nextLineSize = lineSizes[sameSizeIdx+1][2]
    end


    --// try to fillup the shorest lines to the 2. shortest linesize (so we can repeat the process on n+1 lines)
    --// (if all/multiple have the same size share the freespace between them)
    local spaceToAlloc = shortestLineSize - nextLineSize
    if (spaceToAlloc > freeSpace) then
      spaceToAlloc = freeSpace
      freeSpace    = 0
    else
      freeSpace    = freeSpace - spaceToAlloc
    end
    local perItemAlloc = (spaceToAlloc / sameSizeIdx)


    --// set the linesizes
    for i=1,sameSizeIdx do
      local linei = lineSizes[i]
      linei[2] = linei[2] + perItemAlloc --FIXME orientation

      --// adjust the top/left startcoord of all following lines (in the line)
      local nextLineIdx = linei[1]+1
      local nextLine = ((nextLineIdx <= #_lines) and _lines[ nextLineIdx ]) or #_cells+1
      for j=_lines[ linei[1] ],nextLine-1 do
        local cell = _cells[j]
        cell[4] = cell[4] + perItemAlloc --FIXME orientation
      end
      for j=nextLine,#_cells do
        local cell = _cells[j]
        cell[2] = cell[2] + perItemAlloc --FIXME orientation
      end
    end

    shortestLineSize = nextLineSize

  end
end

--//=============================================================================

function LayoutPanel:GetMaxWeight()
  --// calculate max weights for each column and row
  local mweightx = {}
  local mweighty = {}

  local cn = self.children

  local dir1,dir2
  if (self.orientation == "vertical") then
    dir1,dir2 = self._columns,self._rows
  else
    dir1,dir2 = self._rows,self._columns
  end

  local n,x,y = 1
  for i=1, dir1 do
    for j=1, dir2 do
      local child = cn[n]
      if not child then break end

      if (self.orientation == "vertical") then
        x,y = i,j
      else
        x,y = j,i
      end

      local we = child.weight or 1
      if ((mweightx[x] or 0.001) < we) then
        mweightx[x] = we
      end
      if ((mweighty[y] or 0.001) < we) then
        mweighty[y] = we
      end
      n = n + 1
    end
  end

  local weightx = table.sum(mweightx)
  local weighty = table.sum(mweighty)
  return mweightx,mweighty,weightx, weighty
end

--//=============================================================================

function LayoutPanel:_GetMaxChildConstraints(child)
  local itemPadding = self.itemPadding
  local margin      = child.margin or self.itemMargin
  local maxChildWidth  = -margin[1] - itemPadding[1] + self.clientWidth  - itemPadding[3] - margin[3]
  local maxChildHeight = -margin[2] - itemPadding[2] + self.clientHeight - itemPadding[4] - margin[4]
  return margin[1] + itemPadding[1], margin[2] + itemPadding[2], maxChildWidth, maxChildHeight
end


function LayoutPanel:UpdateLayout()
  local cn = self.children
  local cn_count = #cn

  if (cn_count==0) then
    return
  end

  self:RealignChildren()

  --FIXME add check if any item.width > maxItemWidth (+Height)

  if (self.resizeItems) then
    local max_ix = math.floor(self.clientArea[3]/self.minItemWidth)
    local max_iy = math.floor(self.clientArea[4]/self.minItemHeight)

    if (max_ix*max_iy < cn_count)or
       (max_ix<(self.columns or 0))or
       (max_iy<(self.rows or 0))
    then
      --FIXME add autoEnlarge/autoAdjustSize?
      --error"LayoutPanel: not enough space"
    end

    --FIXME take minWidth/height maxWidth/Height into account! (and try to reach a 1:1 pixel ratio)
    if self.columns and self.rows then
      self._columns = self.columns
      self._rows    = self.rows
    elseif (not self.columns) and self.rows then
      self._columns = math.ceil(cn_count/self.rows)
      self._rows    = self.rows
    elseif (not self.rows) and self.columns then
      self._columns = self.columns
      self._rows    = math.ceil(cn_count/self.columns)
    else
      local size    = math.ceil(cn_count^0.5)
      self._columns = size
      self._rows    = math.ceil(cn_count/size)
    end

    local childWidth  = self.clientArea[3]/self._columns
    local childHeight = self.clientArea[4]/self._rows
    local childPosx = 0
    local childPosy = 0

    self._cells  = {}
    local _cells = self._cells

    local weightsx,weightsy, maxweightx, maxweighty
    if (self.weightedResize) then
      weightsx,weightsy, maxweightx, maxweighty = self:GetMaxWeight()
      --// special setup for weightedResize
      childWidth = 0
      childHeight = 0
    end

    local dir1,dir2
    if (self.orientation == "vertical") then
      dir1,dir2 = self._columns,self._rows
    else
      dir1,dir2 = self._rows,self._columns
    end

    local n,x,y = 1
    for i=1, dir1 do
      for j=1, dir2 do
        local child = cn[n]
        if not child then break end
        local margin = child.margin or self.itemMargin

        if (self.orientation == "vertical") then
          x,y = i,j
        else
          x,y = j,i
        end

        if (self.weightedResize) then
          --// weighted Position
          if dir1 == 1 then
            childPosx = 0
          else
            childPosx = childPosx + childWidth
          end
          if dir2 == 1 then
            childPosy = 0
          else
            childPosy = childPosy + childHeight
          end
          childWidth  = (self.clientArea[3]) * weightsx[x]/maxweightx
          childHeight = (self.clientArea[4]) * weightsy[y]/maxweighty
        else
          --// position without weightedResize
          childPosx = childWidth * (x-1)
          childPosy = childHeight * (y-1)
        end


        local childBox = {
          childPosx + margin[1],
          childPosy + margin[2],
          childWidth - margin[1] - margin[3],
          childHeight - margin[2] - margin[4]
        }
        child:_UpdateConstraints(unpack4(childBox))
        _cells[n] = childBox
        n = n+1
      end
    end

  else

    local clientAreaWidth,clientAreaHeight = self.clientArea[3],self.clientArea[4]

    self._lines  = {1}
    local _lines = self._lines
    self._cells  = {}
    local _cells = self._cells

    local itemMargin  = self.itemMargin
    local itemPadding = self.itemPadding

    local x,y = 0,0
    local curLine, curLineSize = 1,self.minItemHeight
    local totalChildWidth,totalChildHeight = 0
    local cell_left,cell_top,cell_width,cell_height = 0,0,0,0
    local lineHeights = {}
    local lineWidths = {}

    for i=1, cn_count do
      --FIXME use orientation!

      local child = cn[i]
      if not child then break end
      local margin = child.margin or itemMargin

      local childWidth  = math.max(child.width,self.minItemWidth)
      local childHeight = math.max(child.height,self.minItemHeight)

      totalChildWidth  = margin[1] + itemPadding[1] +  childWidth  + itemPadding[3] + margin[3] --// FIXME add margin just for non-border controls
      totalChildHeight = itemPadding[2] + childHeight + itemPadding[4]

      if (curLine > 1) then
        totalChildHeight = margin[2] + totalChildHeight
      end
      if (i < cn_count) then --// check for lastLine and not last control!
        totalChildHeight = totalChildHeight + margin[4]
      end
      
      cell_top    = y + itemPadding[2]
      if (curLine > 1) then
        cell_top = cell_top + margin[2]
      end
      cell_left   = x + margin[1] + itemPadding[1]
      cell_width  = childWidth
      cell_height = childHeight

      x = x + totalChildWidth

      --FIXME check if the child is too large for the client area
      if (totalChildWidth > clientAreaWidth)
       --or(y+child.height+margin[4] > clientAreaHeight) --FIXME
      then
        --Spring.Echo("Chili-LayoutPanel: item too large (" .. child.name .. ")", totalChildWidth, clientAreaWidth)
        --//todo handle this
      end

      if
	(i>1)and
        (self.columns and ((i+1)%self.columns < 1))or
        ((not self.columns) and (x > clientAreaWidth))
      then
        lineHeights[curLine] = curLineSize
        lineWidths[curLine]  = x - totalChildWidth

        --// start a new line
        x = totalChildWidth
        y = y + curLineSize

        curLine = curLine+1
        curLineSize = math.max(self.minItemHeight,totalChildHeight)
        cell_top  = y + margin[2] + itemPadding[2]
        cell_left = margin[1] + itemPadding[1]
        _lines[curLine] = i
      end

      _cells[i] = {cell_left,cell_top,cell_width,cell_height}

      if (totalChildHeight > curLineSize) then
        curLineSize = totalChildHeight
      end
    end

    lineHeights[curLine] = curLineSize
    lineWidths[curLine]  = x

    if (self.centerItems or self.autoArrangeH or self.autoArrangeV) then
      for i=1,#lineWidths do
        local startcell = _lines[i]
        local endcell   = (_lines[i+1] or (#cn+1)) - 1
        local freespace = clientAreaWidth - lineWidths[i]
        self:_AutoArrangeAbscissa(startcell, endcell, freespace)
      end

      for i=1,#lineHeights do
        local lineHeight = lineHeights[i]
        local startcell  = _lines[i]
        local endcell    = (_lines[i+1] or (#cn+1)) - 1
        self:_EnlargeToLineHeight(startcell, endcell, lineHeight)
      end
      self:_AutoArrangeOrdinate(clientAreaHeight - (y + curLineSize))
    end


    --FIXME do this in resize too!
    if (self.autosize) then
      clientAreaHeight = y + curLineSize
      self.clientHeight = clientAreaHeight
      self.clientArea[4] = clientAreaHeight
      self.height = self.padding[2] + clientAreaHeight +  self.padding[4]
      self:UpdateClientArea() --TODO check if this calls UpateLayoutagain!
    elseif (y+curLineSize > clientAreaHeight) then
      --Spring.Echo(debug.traceback())
    end


    for i=1, cn_count do
      local child = cn[i]
      if not child then break end

      local cell = _cells[i]
      local cposx,cposy = cell[1],cell[2]
      if (self.centerItems) then
        cposx = cposx + (cell[3] - child.width) * 0.5
        cposy = cposy + (cell[4] - child.height) * 0.5
      end

      child:_UpdateConstraints(cposx,cposy)
    end

  end

  self:RealignChildren() --FIXME split SetPos from AlignControl!!!

  return true
end

--//=============================================================================

function LayoutPanel:DrawBackground()
end


function LayoutPanel:DrawItemBkGnd(index)
end


function LayoutPanel:DrawControl()
  self:DrawBackground(self)
end


function LayoutPanel:DrawChildren()
  local cn = self.children
  if (not cn[1]) then return end

  gl.PushMatrix()
  gl.Translate(math.floor(self.x + self.clientArea[1]),math.floor(self.y + self.clientArea[2]),0)
  for i=1,#cn do
    self:DrawItemBkGnd(i)
  end
  if (self.debug) then
    gl.Color(1,0,0,0.5)
    for i=1,#self._cells do
      local x,y,w,h = unpack4(self._cells[i])
      gl.Rect(x,y,x+w,y+h)
    end
  end
  gl.PopMatrix()

  self:_DrawChildrenInClientArea('Draw')
end


function LayoutPanel:DrawChildrenForList()
  local cn = self.children
  if (not cn[1]) then return end

  if (self.debug) then
    gl.Color(0,1,0,0.5)
    gl.PolygonMode(GL.FRONT_AND_BACK,GL.LINE)
    gl.LineWidth(2)
    gl.Rect(self.x,self.y,self.x+self.width,self.y+self.height)
    gl.LineWidth(1)
    gl.PolygonMode(GL.FRONT_AND_BACK,GL.FILL)
  end

  gl.PushMatrix()
  gl.Translate(math.floor(self.x + self.clientArea[1]),math.floor(self.y + self.clientArea[2]),0)
  for i=1,#cn do
    self:DrawItemBkGnd(i)
  end
  if (self.debug) then
    gl.Color(1,0,0,0.5)
    for i=1,#self._cells do
      local x,y,w,h = unpack4(self._cells[i])
      gl.Rect(x,y,x+w,y+h)
    end
  end
  gl.PopMatrix()

  self:_DrawChildrenInClientArea('DrawForList')
end

--//=============================================================================

function LayoutPanel:GetItemIndexAt(cx,cy)
  local cells = self._cells
  local itemPadding = self.itemPadding
  for i=1,#cells do
    local cell  = cells[i]
    local cellbox = ExpandRect(cell,itemPadding)
    if (InRect(cellbox, cx,cy)) then
      return i
    end
  end
  return -1
end


function LayoutPanel:GetItemXY(itemIdx)
  local cell = self._cells[itemIdx]
  if (cell) then
    return unpack4(cell)
  end
end

--//=============================================================================

function LayoutPanel:MultiRectSelect(item1,item2,append)
  --// note: this functions does NOT update self._lastSelected!

  --// select all items in the convex hull of those 2 items
  local cells = self._cells
  local itemPadding = self.itemPadding

  local cell1,cell2 = cells[item1],cells[item2]

  local convexHull = {
    math.min(cell1[1],cell2[1]),
    math.min(cell1[2],cell2[2]),
  }
  convexHull[3] = math.max(cell1[1]+cell1[3],cell2[1]+cell2[3]) - convexHull[1]
  convexHull[4] = math.max(cell1[2]+cell1[4],cell2[2]+cell2[4]) - convexHull[2]

  local newSelected = self.selectedItems
  if (not append) then
    newSelected = {}
  end

  for i=1,#cells do
    local cell  = cells[i]
    local cellbox = ExpandRect(cell,itemPadding)
    if (AreRectsOverlapping(convexHull,cellbox)) then
      newSelected[i] = true
    end
  end

  if (not append) then
    local oldSelected = self.selectedItems
    self.selectedItems = newSelected
    for itemIdx,selected in pairs(oldSelected) do
      if (selected)and(not newSelected[itemIdx]) then
        self:CallListeners(self.OnSelectItem, itemIdx, false)
      end
    end
    for itemIdx,selected in pairs(newSelected) do
      if (selected)and(not oldSelected[itemIdx]) then
        self:CallListeners(self.OnSelectItem, itemIdx, true)
      end
    end
  end

  self:Invalidate()
end


function LayoutPanel:ToggleItem(itemIdx)
  local newstate = not self.selectedItems[itemIdx]
  self.selectedItems[itemIdx] = newstate
  self._lastSelected = itemIdx
  self:CallListeners(self.OnSelectItem, itemIdx, newstate)
  self:Invalidate()
end


function LayoutPanel:SelectItem(itemIdx, append)
  if (self.selectedItems[itemIdx]) then
    return
  end

  if (append) then
    self.selectedItems[itemIdx] = true
  else
    local oldItems = self.selectedItems
    self.selectedItems = {[itemIdx]=true}
    for oldItemIdx in pairs(oldItems) do
      if (oldItemIdx ~= itemIdx) then
        self:CallListeners(self.OnSelectItem, oldItemIdx, false)
      end
    end
  end
  self._lastSelected = itemIdx
  self:CallListeners(self.OnSelectItem, itemIdx, true)
  self:Invalidate()
end


function LayoutPanel:DeselectItem(itemIdx)
  if (not self.selectedItems[itemIdx]) then
    return
  end

  self.selectedItems[itemIdx] = false
  self._lastSelected = itemIdx
  self:CallListeners(self.OnSelectItem, itemIdx, false)
  self:Invalidate()
end


function LayoutPanel:SelectAll()
  for i=1,#self.children do
    self:SelectItem(i, append)
  end
end


function LayoutPanel:DeselectAll()
  for idx in pairs(self.selectedItems) do
    self:DeselectItem(idx)
  end
end


--//=============================================================================

function LayoutPanel:MouseDown(x,y,button,mods)
  local clickedChild = inherited.MouseDown(self,x,y,button,mods)
  if (clickedChild) then
    return clickedChild
  end

  if (not self.selectable) then return end

  --//FIXME HitTest just returns true when we hover a children -> this won't get called when you hit on in empty space!
  if (button==3) then
    self:DeselectAll()
    return self
  end

  local cx,cy = self:LocalToClient(x,y)
  local itemIdx = self:GetItemIndexAt(cx,cy)

  if (itemIdx>0) then
    if (self.multiSelect) then
      if (mods.shift and mods.ctrl) then
        self:MultiRectSelect(itemIdx,self._lastSelected or 1, true)
      elseif (mods.shift) then
        self:MultiRectSelect(itemIdx,self._lastSelected or 1)
      elseif (mods.ctrl) then
        self:ToggleItem(itemIdx)
      else
        self:SelectItem(itemIdx)
      end
    else
      self:SelectItem(itemIdx)
    end

    return self
  end
end


function LayoutPanel:MouseDblClick(x,y,button,mods)
  local clickedChild = inherited.MouseDown(self,x,y,button,mods)
  if (clickedChild) then
    return clickedChild
  end

  if (not self.selectable) then return end

  local cx,cy = self:LocalToClient(x,y)
  local itemIdx = self:GetItemIndexAt(cx,cy)

  if (itemIdx>0) then
    self:CallListeners(self.OnDblClickItem, itemIdx)
    return self
  end
end

--//=============================================================================