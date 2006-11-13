// just for testing

using CSharpAI;

namespace Testing
{
    // calls test classes
    public class RunTests
    {
        public void Go()
        {
           new TestMaps().Go();
           new TestDrawing().Go();
        }
    }
}

