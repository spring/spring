namespace NTaiToolkit
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    public class CKeywords
    {
        public CKeywords()
        {
            this.values = new Dictionary<string, EWordType>();
            this.AddNTaiKeywords();
        }

        public void AddCollection(ICollection c, EWordType e)
        {
            foreach (string text1 in c)
            {
                this.values[text1] = e;
            }
        }

        public void AddNTaiKeywords()
        {
            this.AddWord("b_power - build energy stuff", EWordType.e_keyword);
            this.AddWord("b_mex - build metal extractors", EWordType.e_keyword);
            this.AddWord("b_rand_assault - build a random attack unit", EWordType.e_keyword);
            this.AddWord("b_assault - build the most efficient attacker that is affordable", EWordType.e_keyword);
            this.AddWord("b_factory - build  factory", EWordType.e_keyword);
            this.AddWord("b_builder - build a construction unit", EWordType.e_keyword);
            this.AddWord("b_geo - build a geothermal plant", EWordType.e_keyword);
            this.AddWord("b_scout - build a scouter", EWordType.e_keyword);
            this.AddWord("b_random - build something random", EWordType.e_keyword);
            this.AddWord("b_defence - build a static defense", EWordType.e_keyword);
            this.AddWord("b_radar - build radar", EWordType.e_keyword);
            this.AddWord("b_estore - build energy storage", EWordType.e_keyword);
            this.AddWord("b_targ - build a targeting device", EWordType.e_keyword);
            this.AddWord("b_mstore - build metal storage", EWordType.e_keyword);
            this.AddWord("b_silo - build a building that has stockpiled weapons", EWordType.e_keyword);
            this.AddWord("b_jammer - build a radar jammer", EWordType.e_keyword);
            this.AddWord("b_sonar - build sonar", EWordType.e_keyword);
            this.AddWord("b_antimissile - build an anti missile (nukes) unit", EWordType.e_keyword);
            //this.AddWord("b_artillery - unreliable, dont use", EWordType.e_keyword);
            //this.AddWord("b_focal_mine - build a mine", EWordType.e_keyword);
            this.AddWord("b_sub - build a submarine", EWordType.e_keyword);
            this.AddWord("b_amphib - build an amphibious unit", EWordType.e_keyword);
            this.AddWord("b_mine - build a mine", EWordType.e_keyword);
            this.AddWord("b_carrier - build an aircraft carrier", EWordType.e_keyword);
            this.AddWord("b_metal_maker - build a metal maker", EWordType.e_keyword);
            this.AddWord("b_fortification - build a wall or barrier", EWordType.e_keyword);
            this.AddWord("b_dgun - dgun nearby enemy units", EWordType.e_keyword);
            this.AddWord("b_repair - repair a nearby unit", EWordType.e_keyword);
            this.AddWord("b_randmove - move randomly in any direction", EWordType.e_keyword);
            this.AddWord("b_resurect - issue a large resurrect area command", EWordType.e_keyword);
            this.AddWord("b_rule - a set of construction rules, see documentation", EWordType.e_keyword);
            this.AddWord("b_rule_nofact - a set of construction rules, see documentation", EWordType.e_keyword);
            this.AddWord("b_rule_extreme - same as above but stricter", EWordType.e_keyword);
            this.AddWord("b_rule_extreme_carry - same as above but keeps going till the rules say we need nothing", EWordType.e_keyword);
            this.AddWord("b_rule_extreme_nofact - same as b_rule but without factories", EWordType.e_keyword);
            this.AddWord("b_retreat - issues a move command telling the unit to move towards the nearest allied base", EWordType.e_keyword);
            this.AddWord("b_guardian - keep repairing things until there's nothing to repair or help build within los of the builder", EWordType.e_keyword);
            this.AddWord("b_guardian_mobiles - keep repairing unfinished mobile units until there's nothing to repair or help build within los of the builder", EWordType.e_keyword);
            this.AddWord("b_bomber - build a bomber aircraft", EWordType.e_keyword);
            this.AddWord("b_shield - build a shield unit", EWordType.e_keyword);
            this.AddWord("b_missile_unit - build a unit with a vertical launch missile weapon", EWordType.e_keyword);
            this.AddWord("b_fighter - build a fighter aircraft", EWordType.e_keyword);
            this.AddWord("b_gunship - build a gunship aircraft", EWordType.e_keyword);
            this.AddWord("b_guard_factory - guard the nearest factory", EWordType.e_keyword);
            this.AddWord("b_guard_like_con - guard the nearest builder of the same type", EWordType.e_keyword);
            this.AddWord("b_reclaim - issues a wide area reclaim command", EWordType.e_keyword);
            this.AddWord("b_support - air refeuling, nto currently working", EWordType.e_keyword);
            this.AddWord("b_hub - EE or NOTA style construction hubs", EWordType.e_keyword);
            this.AddWord("b_airsupport - air support, refueling (incomplete)", EWordType.e_keyword);
            this.AddWord("b_offensive_repair_retreat - repair nearby attack units underfire or retreat", EWordType.e_keyword);

            //B_OFFENSIVE_REPAIR_RETREAT,
    	    //B_GUARDIAN_MOBILES,
            //this.AddWord("b_na - halt unit task cycle indefinately", EWordType.e_keyword);
            

            // as now defunct removed 
            //this.AddWord("is_factory - gives this unit the role of a factory", EWordType.e_keyword);
            //this.AddWord("is_builder - gives this unit the role of builder", EWordType.e_keyword);

            this.AddWord("no_rule_interpolation - disables the adding of B_RULE_NO_FACT between every keyword", EWordType.e_keyword);
            this.AddWord("rule_interpolate - enables rule keyword interpolation", EWordType.e_keyword);
            this.AddWord("base_pos - adds this units position to the list of base positions", EWordType.e_keyword);
            this.AddWord("gaia - makes NTai go into gaia mode", EWordType.e_keyword);
            this.AddWord("not_gaia - turn gaia mode off", EWordType.e_keyword);
            this.AddWord("switch_gaia - toggle the gaia setting", EWordType.e_keyword);
        }

        public void AddWord(string s, EWordType e)
        {
            this.values[s] = e;
        }

        public ArrayList GetUniversalKeywords()
        {
            ArrayList list1 = new ArrayList();
            foreach (string text1 in this.values.Keys)
            {
                if (((EWordType) this.values[text1]) == EWordType.e_keyword)
                {
                    list1.Add(text1);
                }
            }
            return list1;
        }


        public Dictionary<string, EWordType> values;
    }
}


