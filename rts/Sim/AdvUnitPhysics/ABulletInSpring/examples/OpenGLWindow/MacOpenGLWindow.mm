#include "MacOpenGLWindow.h"

#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#import <Cocoa/Cocoa.h>
#include "OpenGLInclude.h"


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>


enum
{
	MY_MAC_ALTKEY=1,
	MY_MAC_SHIFTKEY=2,
	MY_MAC_CONTROL_KEY=4
};

/* report GL errors, if any, to stderr */
static void checkError(const char *functionName)
{
    GLenum error;
    while (( error = glGetError() ) != GL_NO_ERROR)
    {
        fprintf (stderr, "GL error 0x%X detected in %s\n", error, functionName);
    }
}

void dumpInfo(void)
{
    printf ("Vendor: %s\n", glGetString (GL_VENDOR));
    printf ("Renderer: %s\n", glGetString (GL_RENDERER));
    printf ("Version: %s\n", glGetString (GL_VERSION));
    printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
    checkError ("dumpInfo");
}





// -------------------- View ------------------------

@interface TestView : NSView
{ 
    NSOpenGLContext* m_context;
    int m_lastWidth;
    int m_lastHeight;
    bool m_requestClose;
    b3ResizeCallback    m_resizeCallback;

}
-(void)drawRect:(NSRect)rect;
-(void) MakeContext:(int) openglVersion;
-(void) MakeCurrent;
-(float) GetWindowWidth;
-(float) GetWindowHeight;
-(BOOL) GetRequestClose;
- (BOOL)windowShouldClose:(id)sender;
-(void) setResizeCallback:(b3ResizeCallback) callback;
-(b3ResizeCallback) getResizeCallback;
-(NSOpenGLContext*) getContext;
@end

float loop;

#define Pi 3.1415

@implementation TestView

- (BOOL)windowShouldClose:(id)sender
{
    m_requestClose = true;
    return false;
}
-(BOOL) GetRequestClose
{
    return m_requestClose;
}
-(float) GetWindowWidth
{
    return m_lastWidth;
}
-(float) GetWindowHeight
{
    return m_lastHeight;
}

-(b3ResizeCallback) getResizeCallback
{
	return m_resizeCallback;
}

-(NSOpenGLContext*) getContext
{
	return m_context;
}
-(void)setResizeCallback:(b3ResizeCallback)callback
{
    m_resizeCallback = callback;
}
-(void)drawRect:(NSRect)rect
{
	if (([self frame].size.width != m_lastWidth) || ([self frame].size.height != m_lastHeight))
	{
		m_lastWidth = [self frame].size.width;
		m_lastHeight = [self frame].size.height;
		
		// Only needed on resize:
		[m_context clearDrawable];
		
//		reshape([self frame].size.width, [self frame].size.height);
        float width = [self frame].size.width;
        float height = [self frame].size.height;
        
        
        // Get view dimensions in pixels
     //   glViewport(0,0,10,10);
        
        if (m_resizeCallback)
        {
            (*m_resizeCallback)(width,height);
        }
    #ifndef NO_OPENGL3 
		NSRect backingBounds = [self convertRectToBacking:[self bounds]];
        GLsizei backingPixelWidth  = (GLsizei)(backingBounds.size.width),
        backingPixelHeight = (GLsizei)(backingBounds.size.height);
        
        // Set viewport
        glViewport(0, 0, backingPixelWidth, backingPixelHeight);
	#else	
       glViewport(0,0,(GLsizei)width,(GLsizei)height);
#endif
	}
	
	[m_context setView: self];
	[m_context makeCurrentContext];
	
	// Draw
	//display();
	
	[m_context flushBuffer];
	[NSOpenGLContext clearCurrentContext];
	
	loop = loop + 0.1;
}

-(void) MakeContext :(int) openglVersion
{
    //	NSWindow *w;
	NSOpenGLPixelFormat *fmt;
    
	m_requestClose = false;

	
	
#ifndef NO_OPENGL3	
	if (openglVersion==3)
	{
		NSOpenGLPixelFormatAttribute attrs[] =
		{
			NSOpenGLPFAOpenGLProfile,
			NSOpenGLProfileVersion3_2Core,
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFADepthSize, 32,
			NSOpenGLPFAStencilSize, (NSOpenGLPixelFormatAttribute)8,
			(NSOpenGLPixelFormatAttribute)0
		};
			
		// Init GL context
		fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*)attrs];
	} else
#endif
	{
		NSOpenGLPixelFormatAttribute attrs[] =
		{
			NSOpenGLPFADoubleBuffer,
			NSOpenGLPFADepthSize, 32,
			NSOpenGLPFAStencilSize, (NSOpenGLPixelFormatAttribute)8,
			(NSOpenGLPixelFormatAttribute)0
		};
		// Init GL context
		fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*)attrs];
		
	}
	m_context = [[NSOpenGLContext alloc] initWithFormat: fmt shareContext: nil];
	[fmt release];
	[m_context makeCurrentContext];
    
	checkError("makeCurrentContext");
}

-(void) MakeCurrent
{
    [m_context makeCurrentContext];
}
-(void)windowWillClose:(NSNotification *)note
{
    [[NSApplication sharedApplication] terminate:self];
}
@end

struct MacOpenGLWindowInternalData
{
    MacOpenGLWindowInternalData()
    {
        m_myApp = 0;
        m_myview = 0;
        m_pool = 0;
        m_window = 0;
        m_width = -1;
        m_height = -1;
        m_exitRequested = false;
    }
    NSApplication*      m_myApp;
    TestView*             m_myview;
    NSAutoreleasePool*  m_pool;
    NSWindow*           m_window;
    int m_width;
    int m_height;
    bool m_exitRequested;
    
};

MacOpenGLWindow::MacOpenGLWindow()
:m_internalData(0),
m_mouseX(0),
m_mouseY(0),
m_modifierFlags(0),
m_mouseMoveCallback(0),
m_mouseButtonCallback(0),
m_wheelCallback(0),
m_keyboardCallback(0),
m_retinaScaleFactor(1)
{
}

MacOpenGLWindow::~MacOpenGLWindow()
{
    if (m_internalData)
        closeWindow();
}


bool    MacOpenGLWindow::isModifierKeyPressed(int key)
{
        bool isPressed = false;

        switch (key)
        {
                case B3G_ALT:
                {
                        isPressed = ((m_modifierFlags & MY_MAC_ALTKEY)!=0);
                        break;
                };
                case B3G_SHIFT:
                {
                        isPressed = ((m_modifierFlags & MY_MAC_SHIFTKEY)!=0);
                        break;
                };
                case B3G_CONTROL:
                {
                        isPressed = ((m_modifierFlags & MY_MAC_CONTROL_KEY )!=0);
                        break;
                };

                default:
                {
                }
        };
        return isPressed;
}

float	MacOpenGLWindow::getTimeInSeconds()
{
	return 0.f;
}


void MacOpenGLWindow::setRenderCallback( b3RenderCallback renderCallback)
{
	m_renderCallback = renderCallback;
}

void MacOpenGLWindow::setWindowTitle(const char* windowTitle)
{
	[m_internalData->m_window setTitle:[NSString stringWithCString:windowTitle encoding:NSISOLatin1StringEncoding]] ;
}

void MacOpenGLWindow::createWindow(const b3gWindowConstructionInfo& ci)
{
    if (m_internalData)
        closeWindow();

    int width = ci.m_width;
	int height = ci.m_height;
	const char* windowTitle = ci.m_title;
	

    
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    m_internalData = new MacOpenGLWindowInternalData;
    m_internalData->m_width = width;
    m_internalData->m_height = height;
    
    m_internalData->m_pool = [NSAutoreleasePool new];
	m_internalData->m_myApp = [NSApplication sharedApplication];
	//myApp = [MyApp sharedApplication];
	//home();
    
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    id menubar = [[NSMenu new] autorelease];
    id appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    
    id appMenu = [[NSMenu new] autorelease];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = [@"Quit " stringByAppendingString:appName];
    id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
                                                  action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
 
    NSMenuItem *fileMenuItem = [[NSMenuItem new] autorelease];
    NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    [fileMenuItem setSubmenu: fileMenu]; // was setMenu:
    
    NSMenuItem *newMenu = [[NSMenuItem alloc] initWithTitle:@"New" action:NULL keyEquivalent:@""];
    NSMenuItem *openMenu = [[NSMenuItem alloc] initWithTitle:@"Open" action:NULL keyEquivalent:@""];
    NSMenuItem *saveMenu = [[NSMenuItem alloc] initWithTitle:@"Save" action:NULL keyEquivalent:@""];

    [fileMenu addItem: newMenu];
    [fileMenu addItem: openMenu];
    [fileMenu addItem: saveMenu];
    [menubar addItem: fileMenuItem];
        
    
    // add Edit menu
    NSMenuItem *editMenuItem = [[NSMenuItem new] autorelease];
    NSMenu *menu = [[NSMenu allocWithZone:[NSMenu menuZone]]initWithTitle:@"Edit"];
    [editMenuItem setSubmenu: menu];
    
    NSMenuItem *copyItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]]initWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
    
    [menu addItem:copyItem];
    [menubar addItem:editMenuItem];
    
   // [mainMenu setSubmenu:menu forItem:menuItem];
    
    
    //NSMenuItem *fileMenuItem = [[NSMenuItem alloc] initWithTitle: @"File"];
    /*[fileMenuItem setSubmenu: fileMenu]; // was setMenu:
    [fileMenuItem release];
    */
    
    /*NSMenu *newMenu;
    NSMenuItem *newItem;
    
    // Add the submenu
    newItem = [[NSMenuItem allocWithZone:[NSMenu menuZone]]
               initWithTitle:@"Flashy" action:NULL keyEquivalent:@""];
    newMenu = [[NSMenu allocWithZone:[NSMenu menuZone]]
               initWithTitle:@"Flashy"];
    [newItem setSubmenu:newMenu];
    [newMenu release];
    [[NSApp mainMenu] addItem:newItem];
    [newItem release];
    */
    
	NSRect frame = NSMakeRect(0., 0., width, height);
	
	m_internalData->m_window = [NSWindow alloc];
	[m_internalData->m_window initWithContentRect:frame
                      styleMask:NSTitledWindowMask |NSResizableWindowMask| NSClosableWindowMask | NSMiniaturizableWindowMask
                        backing:NSBackingStoreBuffered
                          defer:false];
	
	
	[m_internalData->m_window setTitle:[NSString stringWithCString:windowTitle encoding:NSISOLatin1StringEncoding]] ;
	
	m_internalData->m_myview = [TestView alloc];

    [m_internalData->m_myview setResizeCallback:0];
     ///ci.m_resizeCallback];
    
	[m_internalData->m_myview initWithFrame: frame];
    
	// OpenGL init!
	[m_internalData->m_myview MakeContext : ci.m_openglVersion];

   // https://developer.apple.com/library/mac/#documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/CapturingScreenContents/CapturingScreenContents.html#//apple_ref/doc/uid/TP40012302-CH10-SW1
    //support HighResolutionOSX for Retina Macbook
    if (ci.m_openglVersion>=3)
    {
        [m_internalData->m_myview  setWantsBestResolutionOpenGLSurface:YES];
    }
    NSSize sz;
    sz.width = 1;
    sz.height = 1;
    
   //  float newBackingScaleFactor = [m_internalData->m_window backingScaleFactor];
    
    dumpInfo();
    

    
 
    [m_internalData->m_window setContentView: m_internalData->m_myview];

  
    
	[m_internalData->m_window setDelegate:(id) m_internalData->m_myview];
	
    [m_internalData->m_window makeKeyAndOrderFront: nil];
    
    [m_internalData->m_myview MakeCurrent];
    m_internalData->m_width = m_internalData->m_myview.GetWindowWidth;
    m_internalData->m_height = m_internalData->m_myview.GetWindowHeight;
    
    
    [NSApp activateIgnoringOtherApps:YES];
    
   
//[m_internalData->m_window setLevel:NSMainMenuWindowLevel];
    
//    [NSEvent addGlobalMonitorForEventsMatchingMask:NSMouseMovedMask];
    
//    [NSEvent addGlobalMonitorForEventsMatchingMask:NSMouseMovedMask handler:^(NSEvent *event)
  //  {
        //[window setFrameOrigin:[NSEvent mouseLocation]];
      //  NSPoint eventLocation = [m_internalData->m_window mouseLocationOutsideOfEventStream];
        
      //  NSPoint eventLocation = [event locationInWindow];
        //NSPoint center = [m_internalData->m_myview convertPoint:eventLocation fromView:nil];
       // m_mouseX = center.x;
       // m_mouseY = [m_internalData->m_myview GetWindowHeight] - center.y;
        
        
        // printf("mouse coord = %f, %f\n",m_mouseX,m_mouseY);
    //    if (m_mouseMoveCallback)
     //       (*m_mouseMoveCallback)(m_mouseX,m_mouseY);
        
   // }];

    //see http://stackoverflow.com/questions/8238473/cant-get-nsmousemoved-events-from-nexteventmatchingmask-with-an-nsopenglview
/*       ProcessSerialNumber psn;
	GetCurrentProcess(&psn);
     TransformProcessType(&psn, kProcessTransformToForegroundApplication);
     SetFrontProcess(&psn);
    */
#ifndef NO_OPENGL3 
    m_retinaScaleFactor = [m_internalData->m_myview convertSizeToBacking:sz].width;
#else
	m_retinaScaleFactor=1.f;
#endif

     [m_internalData->m_myApp finishLaunching];
    [pool release];

}

void MacOpenGLWindow::runMainLoop()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    // FILE* dump = fopen ("/Users/erwincoumans/yes.txt","wb");
    // fclose(dump);
    
   

    
    [pool release];

}

void MacOpenGLWindow::closeWindow()
{
    
    delete m_internalData;
    m_internalData = 0;
    
}
extern float m_azi;
extern float m_ele;
extern float m_cameraDistance;


 /*
 *  Summary:
 *    Virtual keycodes
 *
 *  Discussion:
 *    These constants are the virtual keycodes defined originally in
 *    Inside Mac Volume V, pg. V-191. They identify physical keys on a
 *    keyboard. Those constants with "ANSI" in the name are labeled
 *    according to the key position on an ANSI-standard US keyboard.
 *    For example, kVK_ANSI_A indicates the virtual keycode for the key
 *    with the letter 'A' in the US keyboard layout. Other keyboard
 *    layouts may have the 'A' key label on a different physical key;
 *    in this case, pressing 'A' will generate a different virtual
 *    keycode.
 */
enum {
	kVK_ANSI_A                    = 0x00,
	kVK_ANSI_S                    = 0x01,
	kVK_ANSI_D                    = 0x02,
	kVK_ANSI_F                    = 0x03,
	kVK_ANSI_H                    = 0x04,
	kVK_ANSI_G                    = 0x05,
	kVK_ANSI_Z                    = 0x06,
	kVK_ANSI_X                    = 0x07,
	kVK_ANSI_C                    = 0x08,
	kVK_ANSI_V                    = 0x09,
	kVK_ANSI_B                    = 0x0B,
	kVK_ANSI_Q                    = 0x0C,
	kVK_ANSI_W                    = 0x0D,
	kVK_ANSI_E                    = 0x0E,
	kVK_ANSI_R                    = 0x0F,
	kVK_ANSI_Y                    = 0x10,
	kVK_ANSI_T                    = 0x11,
	kVK_ANSI_1                    = 0x12,
	kVK_ANSI_2                    = 0x13,
	kVK_ANSI_3                    = 0x14,
	kVK_ANSI_4                    = 0x15,
	kVK_ANSI_6                    = 0x16,
	kVK_ANSI_5                    = 0x17,
	kVK_ANSI_Equal                = 0x18,
	kVK_ANSI_9                    = 0x19,
	kVK_ANSI_7                    = 0x1A,
	kVK_ANSI_Minus                = 0x1B,
	kVK_ANSI_8                    = 0x1C,
	kVK_ANSI_0                    = 0x1D,
	kVK_ANSI_RightBracket         = 0x1E,
	kVK_ANSI_O                    = 0x1F,
	kVK_ANSI_U                    = 0x20,
	kVK_ANSI_LeftBracket          = 0x21,
	kVK_ANSI_I                    = 0x22,
	kVK_ANSI_P                    = 0x23,
	kVK_ANSI_L                    = 0x25,
	kVK_ANSI_J                    = 0x26,
	kVK_ANSI_Quote                = 0x27,
	kVK_ANSI_K                    = 0x28,
	kVK_ANSI_Semicolon            = 0x29,
	kVK_ANSI_Backslash            = 0x2A,
	kVK_ANSI_Comma                = 0x2B,
	kVK_ANSI_Slash                = 0x2C,
	kVK_ANSI_N                    = 0x2D,
	kVK_ANSI_M                    = 0x2E,
	kVK_ANSI_Period               = 0x2F,
	kVK_ANSI_Grave                = 0x32,
	kVK_ANSI_KeypadDecimal        = 0x41,
	kVK_ANSI_KeypadMultiply       = 0x43,
	kVK_ANSI_KeypadPlus           = 0x45,
	kVK_ANSI_KeypadClear          = 0x47,
	kVK_ANSI_KeypadDivide         = 0x4B,
	kVK_ANSI_KeypadEnter          = 0x4C,
	kVK_ANSI_KeypadMinus          = 0x4E,
	kVK_ANSI_KeypadEquals         = 0x51,
	kVK_ANSI_Keypad0              = 0x52,
	kVK_ANSI_Keypad1              = 0x53,
	kVK_ANSI_Keypad2              = 0x54,
	kVK_ANSI_Keypad3              = 0x55,
	kVK_ANSI_Keypad4              = 0x56,
	kVK_ANSI_Keypad5              = 0x57,
	kVK_ANSI_Keypad6              = 0x58,
	kVK_ANSI_Keypad7              = 0x59,
	kVK_ANSI_Keypad8              = 0x5B,
	kVK_ANSI_Keypad9              = 0x5C
};

/* keycodes for keys that are independent of keyboard layout*/
enum {
	kVK_Return                    = 0x24,
	kVK_Tab                       = 0x30,
	kVK_Space                     = 0x31,
	kVK_Delete                    = 0x33,
	kVK_Escape                    = 0x35,
	kVK_Command                   = 0x37,
	kVK_Shift                     = 0x38,
	kVK_CapsLock                  = 0x39,
	kVK_Option                    = 0x3A,
	kVK_Control                   = 0x3B,
	kVK_RightShift                = 0x3C,
	kVK_RightOption               = 0x3D,
	kVK_RightControl              = 0x3E,
	kVK_Function                  = 0x3F,
	kVK_F17                       = 0x40,
	kVK_VolumeUp                  = 0x48,
	kVK_VolumeDown                = 0x49,
	kVK_Mute                      = 0x4A,
	kVK_F18                       = 0x4F,
	kVK_F19                       = 0x50,
	kVK_F20                       = 0x5A,
	kVK_F5                        = 0x60,
	kVK_F6                        = 0x61,
	kVK_F7                        = 0x62,
	kVK_F3                        = 0x63,
	kVK_F8                        = 0x64,
	kVK_F9                        = 0x65,
	kVK_F11                       = 0x67,
	kVK_F13                       = 0x69,
	kVK_F16                       = 0x6A,
	kVK_F14                       = 0x6B,
	kVK_F10                       = 0x6D,
	kVK_F12                       = 0x6F,
	kVK_F15                       = 0x71,
	kVK_Help                      = 0x72,
	kVK_Home                      = 0x73,
	kVK_PageUp                    = 0x74,
	kVK_ForwardDelete             = 0x75,
	kVK_F4                        = 0x76,
	kVK_End                       = 0x77,
	kVK_F2                        = 0x78,
	kVK_PageDown                  = 0x79,
	kVK_F1                        = 0x7A,
	kVK_LeftArrow                 = 0x7B,
	kVK_RightArrow                = 0x7C,
	kVK_DownArrow                 = 0x7D,
	kVK_UpArrow                   = 0x7E
};

/* ISO keyboards only*/
enum {
	kVK_ISO_Section               = 0x0A
};

/* JIS keyboards only*/
enum {
	kVK_JIS_Yen                   = 0x5D,
	kVK_JIS_Underscore            = 0x5E,
	kVK_JIS_KeypadComma           = 0x5F,
	kVK_JIS_Eisu                  = 0x66,
	kVK_JIS_Kana                  = 0x68
};

int getAsciiCodeFromVirtualKeycode(int virtualKeyCode)
{
	int keycode = 0xffffffff;
	
	switch (virtualKeyCode)
	{
			
		case kVK_ANSI_A   : {keycode = 'a'; break;}
		case kVK_ANSI_B   : {keycode = 'b'; break;}
		case kVK_ANSI_C   : {keycode = 'c'; break;}
		case kVK_ANSI_D	  : {keycode = 'd';break;}
		case kVK_ANSI_E   : {keycode = 'e'; break;}
		case kVK_ANSI_F   : {keycode = 'f'; break;}
		case kVK_ANSI_G   : {keycode = 'g'; break;}
		case kVK_ANSI_H   : {keycode = 'h'; break;}
		case kVK_ANSI_I   : {keycode = 'i'; break;}
		case kVK_ANSI_J   : {keycode = 'j'; break;}
		case kVK_ANSI_K   : {keycode = 'k'; break;}
		case kVK_ANSI_L   : {keycode = 'l'; break;}
		case kVK_ANSI_M   : {keycode = 'm'; break;}
		case kVK_ANSI_N   : {keycode = 'n'; break;}
		case kVK_ANSI_O   : {keycode = 'o'; break;}
		case kVK_ANSI_P   : {keycode = 'p'; break;}
		case kVK_ANSI_Q   : {keycode = 'q'; break;}
		case kVK_ANSI_R   : {keycode = 'r'; break;}
		case kVK_ANSI_S   : {keycode = 's';break;}
		case kVK_ANSI_T   : {keycode = 't'; break;}
		case kVK_ANSI_U   : {keycode = 'u'; break;}
		case kVK_ANSI_V   : {keycode = 'v'; break;}
		case kVK_ANSI_W   : {keycode = 'w'; break;}
		case kVK_ANSI_X   : {keycode = 'x'; break;}
		case kVK_ANSI_Y   : {keycode = 'y'; break;}
		case kVK_ANSI_Z   : {keycode = 'z'; break;}

		case kVK_ANSI_1   : {keycode = '1'; break;}
		case kVK_ANSI_2   : {keycode = '2'; break;}
		case kVK_ANSI_3   : {keycode = '3'; break;}
		case kVK_ANSI_4   : {keycode = '4'; break;}
		case kVK_ANSI_5   : {keycode = '5'; break;}
		case kVK_ANSI_6   : {keycode = '6'; break;}
		case kVK_ANSI_7   : {keycode = '7'; break;}
		case kVK_ANSI_8   : {keycode = '8'; break;}
		case kVK_ANSI_9   : {keycode = '9'; break;}
		case kVK_ANSI_0   : {keycode = '0'; break;}
		case kVK_ANSI_Equal : {keycode = '='; break;}
		case kVK_ANSI_Minus  : {keycode = '-'; break;}
			
		case kVK_Tab:		{keycode = 9; break;}
		case kVK_Space:		{keycode=' '; break;}
		case kVK_Escape:	{keycode=27; break;}
		case kVK_Delete:	{keycode=8; break;}
		case kVK_ForwardDelete:	{keycode=B3G_INSERT; break;}
		
			
		case kVK_F1: {keycode = B3G_F1; break;}
		case kVK_F2: {keycode = B3G_F2; break;}
		case kVK_F3: {keycode = B3G_F3; break;}
		case kVK_F4: {keycode = B3G_F4; break;}
		case kVK_F5: {keycode = B3G_F5; break;}
		case kVK_F6: {keycode = B3G_F6; break;}
		case kVK_F7: {keycode = B3G_F7; break;}
		case kVK_F8: {keycode = B3G_F8; break;}
		case kVK_F9: {keycode = B3G_F9; break;}
		case kVK_F10: {keycode = B3G_F10; break;}
		case kVK_F11: {keycode = B3G_F11; break;}
		case kVK_F12: {keycode = B3G_F12; break;}
		case kVK_F13: {keycode = B3G_F13; break;}
		case kVK_F14: {keycode = B3G_F14; break;}
		case kVK_F15: {keycode = B3G_F15; break;}
			
		case kVK_LeftArrow: {keycode = B3G_LEFT_ARROW;break;}
		case kVK_RightArrow: {keycode = B3G_RIGHT_ARROW;break;}
		case kVK_UpArrow: {keycode = B3G_UP_ARROW;break;}
		case kVK_DownArrow: {keycode = B3G_DOWN_ARROW;break;}

		case kVK_PageUp :{keycode = B3G_PAGE_UP;break;}
		case kVK_PageDown :{keycode = B3G_PAGE_DOWN;break;}
		case kVK_End :{keycode = B3G_END;break;}
		case kVK_Home :{keycode = B3G_HOME;break;}
		case kVK_Control: {keycode = B3G_CONTROL;break;}
		case kVK_Option: {keycode = B3G_ALT;break;}	

		case kVK_ANSI_RightBracket   : {keycode = ']'; break;}
		case kVK_ANSI_LeftBracket  : {keycode = '['; break;}
		case kVK_ANSI_Quote   : {keycode = '\''; break;}
		case kVK_ANSI_Semicolon  : {keycode = ';'; break;}
		case kVK_ANSI_Backslash   : {keycode = '\\'; break;}
		case kVK_ANSI_Comma    : {keycode = ','; break;}
		case kVK_ANSI_Slash  : {keycode = '/'; break;}
		case kVK_ANSI_Period   : {keycode = '.'; break;}
		case kVK_ANSI_Grave     : {keycode = '`'; break;}
		case kVK_ANSI_KeypadDecimal  : {keycode = '.'; break;}
		case kVK_ANSI_KeypadMultiply  : {keycode = '*'; break;}
		case kVK_ANSI_KeypadPlus      : {keycode = '+'; break;}
		case kVK_ANSI_KeypadClear    : {keycode = '?'; break;}
		case kVK_ANSI_KeypadDivide   : {keycode = '/'; break;}
		case kVK_ANSI_KeypadEnter   : {keycode = B3G_RETURN; break;}
		case kVK_ANSI_KeypadMinus   : {keycode = '-'; break;}
		case kVK_ANSI_KeypadEquals  : {keycode = '='; break;}
		case kVK_ANSI_Keypad0   : {keycode = '0'; break;}
		case kVK_ANSI_Keypad1   : {keycode = '1'; break;}
		case kVK_ANSI_Keypad2   : {keycode = '2'; break;}
		case kVK_ANSI_Keypad3   : {keycode = '3'; break;}
		case kVK_ANSI_Keypad4   : {keycode = '4'; break;}
		case kVK_ANSI_Keypad5   : {keycode = '5'; break;}
		case kVK_ANSI_Keypad6   : {keycode = '6'; break;}
		case kVK_ANSI_Keypad7   : {keycode = '7'; break;}
		case kVK_ANSI_Keypad8   : {keycode = '8'; break;}
		case kVK_ANSI_Keypad9   : {keycode = '9'; break;}
        case kVK_Return:
        {
            keycode = B3G_RETURN; break;
        }
            
		default:
		{
			
			printf("unknown keycode\n");
		}
	}
	return keycode;
}


void MacOpenGLWindow::startRendering()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    
	GLint err = glGetError();
    assert(err==GL_NO_ERROR);
    
    
    NSEvent *event = nil;
    bool handledEvent = false;
	
    do
    {
        [pool release];
        pool = [[NSAutoreleasePool alloc] init];
        event =        [m_internalData->m_myApp
         nextEventMatchingMask:NSAnyEventMask
         untilDate:[NSDate distantPast]
         inMode:NSDefaultRunLoopMode
         //		  inMode:NSEventTrackingRunLoopMode
         dequeue:YES];
        
		//NSShiftKeyMask              = 1 << 17,
		//NSControlKeyMask
	
		if ([event type] == NSFlagsChanged)
		{
			int modifiers = [event modifierFlags];
			if (m_keyboardCallback)
			{
				if ((modifiers & NSShiftKeyMask))
				{
					m_keyboardCallback(B3G_SHIFT,1);
					m_modifierFlags |= MY_MAC_SHIFTKEY;
				}else
				{
					if (m_modifierFlags& MY_MAC_SHIFTKEY)
					{
						m_keyboardCallback(B3G_SHIFT,0);	
						m_modifierFlags &= ~MY_MAC_SHIFTKEY;
					}
				}
				if (modifiers & NSControlKeyMask)
				{
					m_keyboardCallback(B3G_CONTROL,1);
					m_modifierFlags |= MY_MAC_CONTROL_KEY;
				} else
				{
					if (m_modifierFlags&MY_MAC_CONTROL_KEY)
					{
						m_keyboardCallback(B3G_CONTROL,0);
						m_modifierFlags &= ~MY_MAC_CONTROL_KEY;
					}
				}
				if (modifiers & NSAlternateKeyMask)
				{
					m_keyboardCallback(B3G_ALT,1);
					m_modifierFlags |= MY_MAC_ALTKEY;
				} else
				{
						if (m_modifierFlags&MY_MAC_ALTKEY)
						{
								m_keyboardCallback(B3G_ALT,0);
								m_modifierFlags &= ~MY_MAC_ALTKEY;

						}
				}
				//handle NSCommandKeyMask
				
			}
		}
		if ([event type] == NSKeyUp)
        {
			handledEvent = true;
			
			uint32 virtualKeycode = [event keyCode];
		   
			int keycode = getAsciiCodeFromVirtualKeycode(virtualKeycode);
			// printf("keycode = %d\n", keycode);
			
			if (m_keyboardCallback)
			{
				int state = 0;
				m_keyboardCallback(keycode,state);
			}
		}
        if ([event type] == NSKeyDown)
        {
			handledEvent = true;
			
			if (![event isARepeat])
			{
				uint32 virtualKeycode = [event keyCode];
				
				int keycode = getAsciiCodeFromVirtualKeycode(virtualKeycode);
				//printf("keycode = %d\n", keycode);

				if (m_keyboardCallback)
				{
					int state = 1;
					m_keyboardCallback(keycode,state);
				}
			}
		}


        if (([event type]== NSRightMouseDown) || ([ event type]==NSLeftMouseDown)||([event type]==NSOtherMouseDown))
        {
           // printf("right mouse!");
          //  float mouseX,mouseY;
            
            NSPoint eventLocation = [event locationInWindow];
            NSPoint center = [m_internalData->m_myview convertPoint:eventLocation fromView:nil];
            m_mouseX = center.x;
            m_mouseY = [m_internalData->m_myview GetWindowHeight] - center.y;
            int button=0;
	        switch ([event type])
            {
                case NSLeftMouseDown:
                {
                    button=0;
                    break;
                }
                case NSOtherMouseDown:
                {
                    button=1;
                    break;
                }
                case NSRightMouseDown:
                {
                    button=2;
                    break;
                }
                default:
                {
                
                }
            };
           // printf("mouse coord = %f, %f\n",mouseX,mouseY);
            if (m_mouseButtonCallback)
			{
				//handledEvent = true;
                (*m_mouseButtonCallback)(button,1,m_mouseX,m_mouseY);
            }
        }
		
		
        if (([event type]== NSRightMouseUp) || ([ event type]==NSLeftMouseUp)||([event type]==NSOtherMouseUp))
        {
			// printf("right mouse!");
			//  float mouseX,mouseY;
            
            NSPoint eventLocation = [event locationInWindow];
            NSPoint center = [m_internalData->m_myview convertPoint:eventLocation fromView:nil];
            m_mouseX = center.x;
            m_mouseY = [m_internalData->m_myview GetWindowHeight] - center.y;
	        
            int button=0;
            switch ([event type])
            {
                case NSLeftMouseUp:
                {
                    button=0;
                    break;
                }
                case NSOtherMouseUp:
                {
                    button=1;
                    break;
                }
                case NSRightMouseUp:
                {
                    button=2;
                    break;
                }
                default:
                {
                    
                }
            };
            
			// printf("mouse coord = %f, %f\n",mouseX,mouseY);
            if (m_mouseButtonCallback)
                (*m_mouseButtonCallback)(button,0,m_mouseX,m_mouseY);
            
        }
		
        
        if ([event type] == NSMouseMoved)
        {
            
            NSPoint eventLocation = [event locationInWindow];
            NSPoint center = [m_internalData->m_myview convertPoint:eventLocation fromView:nil];
            m_mouseX = center.x;
            m_mouseY = [m_internalData->m_myview GetWindowHeight] - center.y;
       
            
           // printf("mouse coord = %f, %f\n",m_mouseX,m_mouseY);
            if (m_mouseMoveCallback)
			{
				//handledEvent = true;
                (*m_mouseMoveCallback)(m_mouseX,m_mouseY);
			}
        }
        
        if (([event type] == NSLeftMouseDragged) || ([event type] == NSRightMouseDragged) || ([event type] == NSOtherMouseDragged))
        {
            int dx1, dy1;
            CGGetLastMouseDelta (&dx1, &dy1);
        
            NSPoint eventLocation = [event locationInWindow];
            NSPoint center = [m_internalData->m_myview convertPoint:eventLocation fromView:nil];
            m_mouseX = center.x;
            m_mouseY = [m_internalData->m_myview GetWindowHeight] - center.y;
            
            if (m_mouseMoveCallback)
            {
				//handledEvent = true;
                (*m_mouseMoveCallback)(m_mouseX,m_mouseY);
            }

          //  printf("mouse coord = %f, %f\n",m_mouseX,m_mouseY);
        }
        
        if ([event type] == NSScrollWheel)
        {
            float dy, dx;
            dy = [ event deltaY ];
            dx = [ event deltaX ];
            
            if (m_wheelCallback)
			{
				handledEvent = true;
                (*m_wheelCallback)(dx,dy);
			}
          //  m_cameraDistance -= dy*0.1;
            // m_azi -= dx*0.1;
            
        }

		if (!handledEvent)
			[m_internalData->m_myApp sendEvent:event];
        
		[m_internalData->m_myApp updateWindows];
    } while (event);
  
	err = glGetError();
    assert(err==GL_NO_ERROR);
    
    [m_internalData->m_myview MakeCurrent];
    err = glGetError();
    assert(err==GL_NO_ERROR);
    
    
   // glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);     //clear buffers

    err = glGetError();
    assert(err==GL_NO_ERROR);
    
    //glCullFace(GL_BACK);
    //glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    err = glGetError();
    assert(err==GL_NO_ERROR);
    
    float aspect;
    //b3Vector3 extents;
    
    if (m_internalData->m_width > m_internalData->m_height)
    {
        aspect = (float)m_internalData->m_width / (float)m_internalData->m_height;
        //extents.setValue(aspect * 1.0f, 1.0f,0);
    } else
    {
        aspect = (float)m_internalData->m_height / (float)m_internalData->m_width;
        //extents.setValue(1.0f, aspect*1.f,0);
    }
    
    err = glGetError();
    assert(err==GL_NO_ERROR);
     [pool release];

}

void MacOpenGLWindow::endRendering()
{
    [m_internalData->m_myview MakeCurrent];
//#ifndef NO_OPENGL3
//	glSwapAPPLE();
//#else
 [[m_internalData->m_myview getContext] flushBuffer];
//  #endif 

}

bool MacOpenGLWindow::requestedExit() const
{
    bool closeme = m_internalData->m_myview.GetRequestClose;
    return m_internalData->m_exitRequested || closeme;
}

void MacOpenGLWindow::setRequestExit()
{
	m_internalData->m_exitRequested = true;
}

#include <string.h>
int MacOpenGLWindow::fileOpenDialog(char* filename, int maxNameLength)
{
    //save/restore the OpenGL context, NSOpenPanel can mess it up
    //http://stackoverflow.com/questions/13987148/nsopenpanel-breaks-my-sdl-opengl-app
    
    NSOpenGLContext *foo = [NSOpenGLContext currentContext];
    // get the url of a .txt file
    NSOpenPanel * zOpenPanel = [NSOpenPanel openPanel];
	NSArray * zAryOfExtensions = [NSArray arrayWithObjects:@"urdf",@"bullet",nil];
    [zOpenPanel setAllowedFileTypes:zAryOfExtensions];
    NSInteger zIntResult = [zOpenPanel runModal];
    
    [foo makeCurrentContext];
    
    if (zIntResult == NSFileHandlingPanelCancelButton) {
        NSLog(@"readUsingOpenPanel cancelled");
        return 0;
    }
    NSURL *zUrl = [zOpenPanel URL];
   if (zUrl)
   {
       //without the file://
       NSString *myString = [zUrl absoluteString];
       int slen = [myString length];
       if (slen < maxNameLength)
       {
           const char *cfilename=[myString UTF8String];
           //expect file:// at start of URL
           const char* p = strstr(cfilename, "file://");
            if (p==cfilename)
            {
                int actualLen = strlen(cfilename)-7;
                memcpy(filename, cfilename+7,actualLen);
                filename[actualLen]=0;
                return actualLen;
            }
       }
   }

    return 0;
}



void MacOpenGLWindow::getMouseCoordinates(int& x, int& y)
{
    
    NSPoint pt = [m_internalData->m_window mouseLocationOutsideOfEventStream];
    m_mouseX = pt.x;
    m_mouseY = pt.y;
    
    x = m_mouseX;
    //our convention is x,y is upper left hand side
    y = [m_internalData->m_myview GetWindowHeight]-m_mouseY;

    
}

void MacOpenGLWindow::setResizeCallback(b3ResizeCallback resizeCallback)
{
    [m_internalData->m_myview setResizeCallback:resizeCallback];
    if (resizeCallback)
    {
        (resizeCallback)(m_internalData->m_width,m_internalData->m_height);
    }
}

b3ResizeCallback MacOpenGLWindow::getResizeCallback()
{
	return [m_internalData->m_myview getResizeCallback];
}

