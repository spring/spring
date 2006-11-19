// *** This is a generated file; if you want to change it, please change CSAIInterfaces.dll, which is the reference
// 
// This file was generated by MonoAbicWrappersGenerator, by Hugh Perkins hughperkins@gmail.com http://manageddreams.com
// 

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;

namespace CSharpAI
{
    public class FeatureDef : MarshalByRefObject, IFeatureDef
    {
        public IntPtr self = IntPtr.Zero;
        public FeatureDef( IntPtr self )
        {
           this.self = self;
        }
      public System.String myName
      {
          get{ return ABICInterface.FeatureDef_get_myName( self ); }
      }

      public System.String description
      {
          get{ return ABICInterface.FeatureDef_get_description( self ); }
      }

      public System.Double metal
      {
          get{ return ABICInterface.FeatureDef_get_metal( self ); }
      }

      public System.Double energy
      {
          get{ return ABICInterface.FeatureDef_get_energy( self ); }
      }

      public System.Double maxHealth
      {
          get{ return ABICInterface.FeatureDef_get_maxHealth( self ); }
      }

      public System.Double radius
      {
          get{ return ABICInterface.FeatureDef_get_radius( self ); }
      }

      public System.Double mass
      {
          get{ return ABICInterface.FeatureDef_get_mass( self ); }
      }

      public System.Boolean upright
      {
          get{ return ABICInterface.FeatureDef_get_upright( self ); }
      }

      public System.Int32 drawType
      {
          get{ return ABICInterface.FeatureDef_get_drawType( self ); }
      }

      public System.Boolean destructable
      {
          get{ return ABICInterface.FeatureDef_get_destructable( self ); }
      }

      public System.Boolean blocking
      {
          get{ return ABICInterface.FeatureDef_get_blocking( self ); }
      }

      public System.Boolean burnable
      {
          get{ return ABICInterface.FeatureDef_get_burnable( self ); }
      }

      public System.Boolean floating
      {
          get{ return ABICInterface.FeatureDef_get_floating( self ); }
      }

      public System.Boolean geoThermal
      {
          get{ return ABICInterface.FeatureDef_get_geoThermal( self ); }
      }

      public System.String deathFeature
      {
          get{ return ABICInterface.FeatureDef_get_deathFeature( self ); }
      }

      public System.Int32 xsize
      {
          get{ return ABICInterface.FeatureDef_get_xsize( self ); }
      }

      public System.Int32 ysize
      {
          get{ return ABICInterface.FeatureDef_get_ysize( self ); }
      }

#line 0 "AbicIFeatureDefWrapper_manualtweaks.txt"

    }
}
