    
namespace CSharpAI
{
    public interface IFeatureDef
    {
        string myName{ get; }
        string description{ get; }
    
        double metal{ get; }
        double energy{ get; }
        double maxHealth{ get; }
    
        //double radius{ get; }
        double mass{ get; }									//used to see if the object can be overrun
    
        bool upright{ get; }
        int drawType{ get; }
        //S3DOModel* model;						//used by 3do obects
        //std::string modelname;			//used by 3do obects
        //int modelType;							//used by tree etc
    
        bool destructable{ get; }
        bool blocking{ get; }
        bool burnable{ get; }
        bool floating{ get; }
    
        bool geoThermal{ get; }
    
        string deathFeature{ get; }		//name of feature that this turn into when killed (not reclaimed)
    
        int xsize{ get; }									//each size is 8 units
        int ysize{ get; }									//each size is 8 units
    }
}
