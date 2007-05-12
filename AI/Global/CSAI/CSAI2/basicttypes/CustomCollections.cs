// Copyright Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 2 as published by the
// Free Software Foundation;
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//

using System;
using System.Collections;

public class StringArrayList : ArrayList{
    public StringArrayList( string[] array )
    {
        if( array != null )
        {
            foreach( string value in array )
            {
                base.Add( value );
            }
        }
    }
    public StringArrayList() :
        base()
    {
    }
    public new virtual string this[ int index ]{get{return (string)base[ index ];}set{base[ index ] = value;}}
    public class StringEnumerator{
        IEnumerator enumerator;
        public StringEnumerator( IEnumerator enumerator ){this.enumerator = enumerator;}
        public string Current{get{return (string)enumerator.Current;}}
        public void MoveNext(){enumerator.MoveNext();}
        public void Reset(){enumerator.Reset();}
    }
    public new StringEnumerator GetEnumerator()
    {
        return new StringEnumerator( base.GetEnumerator() );
    }
    public new string[] ToArray()
    {
        return (string[])base.ToArray( typeof( string ) );
    }
    public override string ToString()
    {
        string result = "";
        for( int i = 0; i < base.Count; i++ )
        {
            result += base[i] + ",";
        }
        return result;
    }
}

public class StringHashtable : Hashtable{
    public virtual string this[ string index ]{get{return (string)base[ index ];}set{base[ index ] = value;}}
}

public class IntArrayList : ArrayList{
    public new virtual int this[ int index ]{get{return (int)base[ index ];}set{base[ index ] = value;}}
    public class IntEnumerator{
        IEnumerator enumerator;
        public IntEnumerator( IEnumerator enumerator ){this.enumerator = enumerator;}
        public int Current{get{return (int)enumerator.Current;}}
        public bool MoveNext(){ return enumerator.MoveNext();}
        public void Reset(){enumerator.Reset();}
    }
    public new IntEnumerator GetEnumerator()
    {
        return new IntEnumerator( base.GetEnumerator() );
    }
    public new int[] ToArray()
    {
        return (int[])base.ToArray( typeof( int ) );
    }
}

