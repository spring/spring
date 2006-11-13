

namespace CSharpAI
{
    public interface IStrategy
    {
        void Tick();
        double GetEffectivenessEstimate();
    }
}
