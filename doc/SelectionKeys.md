# The `select` command

You can bind the command `select` to a key, in order to define custom selection commands. After select, you can specify a **selector**, that defines *which* units will be selected.

This selector consists of three parts: `SOURCE+FILTER+CONCLUSION+`

- *SOURCE*: Which units to choose from.
- *FILTER*: Narrow down the list of units.
- *CONCLUSION*: What to do exactly with the units that were chosen and filtered.

These three parts are all succeeded by a literal plus sign (`+`), so there is one `+` between *SOURCE* and *FILTER*, one between *FILTER* and *CONCLUSION*, and one after *CONCLUSION*. Do **not** use a space (` `) anywhere in the entire expression.

For example, `AllMap+_Builder+_SelectAll+` is a valid selector, where:

- `AllMap` is the *SOURCE*
- `_Builder` is the *FILTER* and
- `_SelectAll` is the *CONCLUSION*.

Note that there is an **underscore** (`_`) between each element, even if there is already a plus sign (`+`), for hysterical raisins...

## Source
The *SOURCE* describes the set of units that you want to filter and pick a selection from. For players, these are restricted your team. It can be one of

- `AllMap`: All active units on the entire map.
- `Visible`: All active units that are currently visible.
- `FromMouse_<float>`: all units that are at most a distance of `<float>` away from the mouse cursor. 
- `FromMouseC_<float>`: same as above, but using a vertical cylinder instead of a sphere. This is good for selecting airplanes or ships on deep water.
- `PrevSelection`: the previous selection; that is, the one active before you hit the selection key 

## Filter
The *FILTER* is an arbitrarily long list of filters. 

Here are the filters. Note that "units" generally means both buildings and mobile units. Typing both got old real quick.

### `Not`

  Every filter can be preceded by `Not` to negate it. You have to use a `_` to separate the `Not` from the filter, as in `Not_Builder`.
  
### `AbsoluteHealth_<float>`

  Keep only units that have an absolute health greater than `<float>` points.
  
  - `AbsoluteHealth_100`: Keep only units that have **more** than 100 health points left.
  - `Not_AbsoluteHealth_100`: Keep only units that have **less** than 101 health points left.

### `Aircraft`
  
  Keep only units that can fly.

### `Builder`
  
  Keep only units and buildings that can construct. This means Factories, Construction Turrets, Constructors, but not Rezzers.

### `Building`
  
  Keep only buildings, not mobile units.

### `Category_<category>`
  
  Keep only units of category `<category>`

### `Guarding`
  
  Keep only units that currently have a **Guard** order.

### `IdMatches_<string>`

  Keep only units whose internal name (unitDef name) matches `<string>` **exactly**.
  
  - `IdMatches_armcom`: keep only Armada Commanders (internally named `armcom`).

### `Idle`
  
  Keep only units that are currently idle, i.e. do not have any active order.

### `InHotkeyGroup`
  
  Keep only units that are in any hotkey group.

  - `Not_InHotkeyGroup`: keep all units that are **not** currently in any hotkey group.

### `InPrevSel`
  
  Keep only units of the same type (unitDefID) as any unit in the selection before this `select` command was run.

### `Jammer`
  
  Keep only units that have a jammer radius greater than 0.

### `ManualFireUnit`:

  Keep only units that have a weapon that requires manual firing (currently only the commanders and Armada Thor).

### `NameContain_<string>`
  
  Keep only units whose name contains the `<string>`.

### `Radar`
  
  Keep only units that have a radar or sonar radius greater than 0.

### `RelativeHealth_<float>`

  Keep only units that have health greater than `<float>` percent.
  
  Example:
    - `Not_RelativeHealth_10`: Keep only units that have less than 10% health.

### `RulesParamEquals_<string>_<integer>`

  Keep only units where the `<string>` rules parameter has the exact value `<integer>`.

### `Transport`
  
  Keep only units that can transport other units.

### `Waiting`
  
  Keep only units that currently have a **Wait** order.

### `WeaponRange_<float>`
  
  Keep only units that have a maximum weapon range greater than `<float>`.

### `Weapons`
  
  Keep only units that have any weapons.



## Conclusion
The *CONCLUSION* specifies what to do with the units that are left from the source after running through all the filters.

If the *CONCLUSION* starts with `_ClearSelection`, your new selection will replace the old one; otherwise, it will just add to it. It must be followed by exactly one of:

- `SelectAll`: all units
- `SelectOne`: one unit, will also center the camera on that unit. This command remembers which unit was selected, on repeating the selection command, the **next** unit will be selected, so you can cycle through all matching units.
- `SelectClosestToCursor`: one unit, the one closest to the mouse cursor.
- `SelectNum_<integer>`: `<integer>` units. Repeating this command adds `<integer>` more units.
- `SelectPart_<float>`: `<float>` percent of the units.


# Examples
Recall that between every two tokens, there must be an underscore `_`, even if there is also a `+`. Another way to put it is that before every word in your selector except the *SOURCE*, there must be an underscore.

Some examples. Again, "unit" also includes buildings.

- `AllMap++_ClearSelection_SelectAll+`

  Selects everything on the entire map.

- `AllMap+_Builder_Idle+_ClearSelection_SelectOne+`
  
  Selects any (one) idle builder (unit or building) on entire map. Repeatedly running this command will cycle through all idle builders.

- `AllMap+_Radar+_ClearSelection_SelectAll+`

  Selects all units with radar/sonar/jammer.

- `AllMap+_Not_Aircraft_Weapons+_ClearSelection_SelectAll+`

  Selects all non-aircraft armed units

- `AllMap+_InPrevSel+_ClearSelection_SelectAll+`

  Selects all units of any type that was in your previous selection.

  Note that up to now, all keys said `ClearSelection`, hence they replaced your old selection.

- `AllMap+_InPrevSel_Not_InHotkeyGroup+_SelectAll+`

  Selects all units of any type that was in your previous selection, unless they are already in a hotkey group.

- `PrevSelection+_Not_Building_Not_RelativeHealth_30+_ClearSelection_SelectAll+`

  From your previous selection, leaves everything that is below 30% health, and not a building. (Use this to quickly retreat damaged units.)
