// for use with Mono, to allow .reloadai and AI separation

namespace CSharpAI
{
    public interface IMonoLoaderProxy
    {
        GlobalAIProxy DynLoad( string assemblyfilename, string debugfilename, string targettypename, string methodname );
    }
}
