--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- A Lua List Object


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Usage:
--   local list = CreateList("name")
--   list:Insert(owner, data)
--   for owner, data in list:iter() do ... end

	--// use array indices for most used entries (faster & less mem usage)
	local DATA  = 1
	local NEXT  = 2
	local PREV  = 3

	local function List_InsertNewListItem(self, owner, data, prev, next)
		local newItem = {
			owner  = owner; --// external code uses this too, so don't hide it behind a magic index
			[DATA]  = data;
			[NEXT]  = next;
			[PREV]  = prev
		}
		if (prev) then prev[NEXT] = newItem end
		if (next) then next[PREV] = newItem end
		if (prev == self.last)  then self.last  = newItem end
		if (next == self.first) then self.first = newItem end
		--return newItem
	end


	local function List_InsertItem(self, owner, data)
		local layer = owner._info.layer
		local item = self.first

		while (item) do
			if (item.owner == owner) then
				return --// already in table
			end

			if self._sortfunc(owner._info, item.owner._info) then
				break
			end
			item = item[NEXT]
		end

		if (item) then
			List_InsertNewListItem(self, owner, data, item[PREV], item)
		else
			List_InsertNewListItem(self, owner, data, self.last, nil) --// end of the pack
		end

		return true
	end


	local function List_RemoveListItem(self, item)
		local prev = item[PREV]
		local next = item[NEXT]

		if (prev) then
			prev[NEXT] = next
		end
		if (next) then
			next[PREV] = prev
		end

		if (self.last == item) then
			self.last = prev
		end
		if (self.first == item) then
			self.first = next
		end
	end


	local function List_RemoveItem(self, owner)
		local item = self.first
		if (not item) then
			return --// list is empty
		end

		while (item) do
			if (item.owner == owner) then
				List_RemoveListItem(self, item)
				return true
			end
			item = item[NEXT]
		end
	end


	local function List_Iter(self, item)
		--// ATTENTION
		--// This iterator works fine when elements get removed during looping
		--// AS LONG AS you don't remove both the current and its succeeding one!!!
		if (item == nil) then
			item = self.first
		else
			item = item[NEXT]
		end
		if (item) then
			return item, item[DATA]
		else
			return nil
		end
	end


	local function List_GetIter(self, it)
		return List_Iter, self, it
	end


	local function List_ReverseIter(self, item)
		--// ATTENTION
		--// This iterator works fine when elements get removed during looping
		--// AS LONG AS you don't remove both the current and its succeeding one!!!
		if (item == nil) then
			item = self.last
		else
			item = item[PREV]
		end
		if (item) then
			return item, item[DATA]
		else
			return nil
		end
	end


	local function List_GetReverseIter(self, it)
		return List_ReverseIter, self, it
	end

CreateList = function(name, sortfunc)
	return {
		name   = name,
		first  = nil,
		last   = nil,
		Insert = List_InsertItem,
		Remove = List_RemoveItem,
		iter      = List_GetIter,
		rev_iter  = List_GetReverseIter,
		_sortfunc = sortfunc,
	}
end
