--[[
Copyright (c) 2010 Matthias Richter

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Except as contained in this notice, the name(s) of the above copyright holders
shall not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
]]--

local baseproxy = newproxy(true)
Vector = getmetatable(baseproxy)

local data = {}
setmetatable(data,{__mode="k"})
local recycle = {}
local recycleN = 0

local function push(t,v)
  recycleN = recycleN + 1
  t[recycleN] = v
end

local function pop(t)
  if (recycleN > 0) then
    local v = t[recycleN]
    t[recycleN] = nil
    recycleN = recycleN - 1
    return v
  else
    return {}
  end
end

function isvector(v)
	return getmetatable(v) == Vector
end

function Vector:__index(i)
  if     (i==1)or(i=="x") then
    return self:unpack()
  elseif (i==2)or(i=="y") then
    return select(2, self:unpack())
  else
    return Vector[i]
  end
end

function Vector:__newindex(i,v)
  if     (i==1)or(i=="x") then
    self:SetX(v)
  elseif (i==2)or(i=="y") then
    self:SetY(v)
  end
end

function Vector:__gc()
	local v = data[self]
	push(recycle, v)
	data[self] = nil
end

function new_vector(x,y)
	local v = newproxy(baseproxy)
	v:Set(x,y)
	return v
end

function Vector:New(x,y)
	return new_vector(x,y)
end

function Vector:clone()
	return new_vector(self.x, self.y)
end

function Vector:Set(x,y)
	local v = data[self]

	if (not v) then
		v = pop(recycle)
		data[self] = v
	end

	v[1] = x
	v[2] = y
end

function Vector:SetX(x)
	local v = data[self]
	v[1] = x
end

function Vector:SetY(y)
	local v = data[self]
	v[2] = y
end

function Vector:unpack()
	local v = data[self]
	return v[1], v[2]
end

--function Vector:__tostring()
--	return "("..tonumber(self.x)..","..tonumber(self.y)..")"
--end

function Vector.__unm(a)
	return new_vector(-a.x, -a.y)
end

function Vector.__add(a,b)
	assert(isvector(a) and isvector(b), "Add: wrong argument types (<vector> expected)")
	return new_vector(a.x+b.x, a.y+b.y)
end

function Vector.__sub(a,b)
	assert(isvector(a) and isvector(b), "Sub: wrong argument types (<vector> expected)")
	return new_vector(a.x-b.x, a.y-b.y)
end

function Vector.__mul(a,b)
	if type(a) == "number" then
		return new_vector(a*b.x, a*b.y)
	elseif type(b) == "number" then
		return new_vector(b*a.x, b*a.y)
	else
		assert(isvector(a) and isvector(b), "Mul: wrong argument types (<vector> or <number> expected)")
		return a.x*b.x + a.y*b.y
	end
end

function Vector.__div(a,b)
	assert(isvector(a) and type(b) == "number", "wrong argument types (expected <vector> / <number>)")
	return new_vector(a.x / b, a.y / b)
end

function Vector.__eq(a,b)
	return a.x == b.x and a.y == b.y
end

function Vector.__lt(a,b)
	return a.x < b.x or (a.x == b.x and a.y < b.y)
end

function Vector.__le(a,b)
	return a.x <= b.x and a.y <= b.y
end

function Vector.permul(a,b)
	assert(isvector(a) and isvector(b), "permul: wrong argument types (<vector> expected)")
	return new_vector(a.x*b.x, a.y*b.y)
end

function Vector:len2()
	return self * self
end

function Vector:len()
	return math.sqrt(self*self)
end

function Vector.dist(a, b)
	assert(isvector(a) and isvector(b), "dist: wrong argument types (<vector> expected)")
	return (b-a):len()
end

function Vector:normalize_inplace()
	local l = self:len()
	self.x, self.y = self.x / l, self.y / l
	return self
end

function Vector:normalized()
	return self / self:len()
end

function Vector:rotate_inplace(phi)
	local c, s = math.cos(phi), math.sin(phi)
	self.x, self.y = c * self.x - s * self.y, s * self.x + c * self.y
	return self
end

function Vector:rotated(phi)
	return self:clone():rotate_inplace(phi)
end

function Vector:perpendicular()
	return new_vector(-self.y, self.x)
end

function Vector:projectOn(v)
	assert(isvector(v), "invalid argument: cannot project onto anything other than a vector.")
	return (self * v) * v / v:len2()
end

function Vector:cross(other)
	assert(isvector(other), "cross: wrong argument types (<vector> expected)")
	return self.x * other.y - self.y * other.x
end
