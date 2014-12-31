#include <windows.h>
#include <stdio.h>

/* Set the visual styles for code compiling in Visual Studio */
#ifdef _MSC_VER
#pragma comment(linker,"\"/manifestdependency:type='win32' \
							name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
							processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

/* Datatypes */

/* The object types */
enum _objectType {
	OBJECT,
	/* Window types */
	WINDOW,
	DIALOG,
	/* Control types */
	CONTROL,
	BUTTON,
	LABEL,
	TEXTBOX
};

/* An event sync mode */
enum _syncMode {
	SYNC,
	ASYNC
};

/* An event's arguments */
struct _eventArgs{
	BOOL PLACEHOLDER; //TODO!
};

/* An event */
struct _event {
	void (*eventFunction)(void*, void*, struct _eventArgs*);
	enum _syncMode mode;
	DWORD message;
	void *sender;
	void *context;
};

/* TextBox multiline/singleline */
enum _textboxtype {
	MULTILINE = TRUE,
	SINGLELINE = FALSE
};


/* =============================================OBJECT DEFINITIONS========================================================= */

/* The main object "interface". All other types directly or indirectly "inherit" from it. */
struct _guiobject {
	void *derived; /* The derived type */

	CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */

	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */
	HINSTANCE *moduleInstance; /* The current module instance */
	char **className; /* The name of the window/control's WinAPI "class" */
	HMENU *ID; /* The child-window/control identifier */
	DWORD *styles; /* The window/control styles bitfield */
	DWORD *exStyles; /* The window/control extended styles bitfield */

	enum _objectType *type; /* The window/control type */
	
	/* events */
	struct _event **events;
	unsigned int *numEvents;

	/* properties */
	char **text; /* The window/control text */

	int *width;
	int *height;

	int *x;
	int *y;

	BOOL *enabled;

	/* links */
	struct _guiobject **parent; /* A pointer to the parent */
	struct _guiobject ***children; /* A pointer to the array of children */
	unsigned int *numChildren; /* The number of children */

	/* methods */
	BOOL (*setPos)(int, int);
	BOOL (*setPosT)(struct _guiobject*, int, int);

	BOOL (*setSize)(int, int);
	BOOL (*setSizeT)(struct _guiobject*, int, int);

	BOOL (*setEvent)(DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);
	BOOL (*setEventT)(struct _guiobject*, DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);

	BOOL (*setText)(char *text);
	BOOL (*setTextT)(struct _guiobject*, char *text);

	BOOL (*setEnabled)(BOOL);
	BOOL (*setEnabledT)(struct _guiobject*, BOOL);

	//TODO (events): setOnSizeChange, setOnPosChange, setOnRepaint, setOnRightclick
};


/* The Window type. Inherits from the GUIObject interface */
struct _window {
	struct _guiobject *base; /* the base type */
	void *derived; /* The derived type */

	CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */

	/* ---------------------INHERITED------------------------------------------------------- */

	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */
	HINSTANCE *moduleInstance; /* The current module instance */
	char **className; /* The name of the window/control's WinAPI "class" */
	HMENU *ID; /* The child-window/control identifier */
	DWORD *styles; /* The window/control styles bitfield */
	DWORD *exStyles; /* The window/control extended styles bitfield */

	enum _objectType *type; /* The window/control type */
	
	/* events */
	struct _event **events;
	unsigned int *numEvents;

	/* properties */
	char **text; /* The window/control text */

	int *width;
	int *height;

	int *x;
	int *y;

	BOOL *enabled;

	/* links */
	struct _guiobject **parent; /* A pointer to the parent */
	struct _guiobject ***children; /* A pointer to the array of children */
	unsigned int *numChildren; /* The number of children */

	/* methods */
	BOOL (*setPos)(int, int);
	BOOL (*setPosT)(struct _guiobject*, int, int);

	BOOL (*setSize)(int, int);
	BOOL (*setSizeT)(struct _guiobject*, int, int);

	BOOL (*setEvent)(DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);
	BOOL (*setEventT)(struct _guiobject*, DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);

	BOOL (*setText)(char *text);
	BOOL (*setTextT)(struct _guiobject*, char *text);

	BOOL (*setEnabled)(BOOL);
	BOOL (*setEnabledT)(struct _guiobject*, BOOL);

	/* -----------------NOT INHERITED------------------------------------------------------- */

	/* properties */
	int *clientWidth;
	int *clientHeight;

	BOOL *resizable;
	BOOL *maximizeEnabled;

	/* methods */
	BOOL (*addChild)(struct _control*); /* Add a child control to the window */
	BOOL (*addChildT)(struct _window*, struct _control*); /* Version without self-reference mechanism */
	
	BOOL (*setOnClick)(void(*)(struct _window*, void*, struct _eventArgs*), void*, enum _syncMode); /* Set an OnClick event for the window */
	BOOL (*setOnClickT)(struct _window*, void(*)(struct _window*, void*, struct _eventArgs*), void*, enum _syncMode); /* Version without self-reference mechanism */
	
	BOOL (*setResizable)(BOOL);
	BOOL (*setResizableT)(struct _window*, BOOL);

	BOOL (*enableMaximize)(BOOL);
	BOOL (*enableMaximizeT)(struct _window*, BOOL);

	//TODO (events): setOnExit, setOnKeypress
	//TODO (other methods): setFullscreen, makeTopmost
};


/* The Control type. Inherits from the GUIObject interface */
struct _control {
	struct _guiobject *base; /* The base type */
	void *derived; /* The derived type */

	CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */

	/* ---------------------INHERITED------------------------------------------------------- */

	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */
	HINSTANCE *moduleInstance; /* The current module instance */
	char **className; /* The name of the window/control's WinAPI "class" */
	HMENU *ID; /* The child-window/control identifier */
	DWORD *styles; /* The window/control styles bitfield */
	DWORD *exStyles; /* The window/control extended styles bitfield */

	enum _objectType *type; /* The window/control type */

	/* event handlers */
	struct _event **events;
	unsigned int *numEvents;
	BOOL (*setEvent)(DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);
	BOOL (*setEventT)(struct _guiobject*, DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);

	BOOL (*setText)(char *text);
	BOOL (*setTextT)(struct _guiobject*, char *text);
	
	/* properties */
	char **text; /* The window/control text */

	int *width;
	int *height;

	int *x;
	int *y;

	BOOL *enabled;

	/* links */
	struct _guiobject **parent; /* A pointer to the parent */
	struct _guiobject ***children; /* A pointer to the array of children */
	unsigned int *numChildren; /* The number of children */

	/* methods */
	BOOL (*setPos)(int, int);
	BOOL (*setPosT)(struct _guiobject*, int, int);

	BOOL (*setSize)(int, int);
	BOOL (*setSizeT)(struct _guiobject*, int, int);

	BOOL (*setEnabled)(BOOL);
	BOOL (*setEnabledT)(struct _guiobject*, BOOL);

	/* -----------------NOT INHERITED------------------------------------------------------- */

	/* methods */
	BOOL (*addChild)(struct _control*); /* Add a child control to the control */
	BOOL (*addChildT)(struct _control*, struct _control*); /* Version without self-reference mechanism */

	BOOL (*addButton)(struct _button*); /* Add a button control to the control (for convenience) */
	BOOL (*addButtonT)(struct _control*, struct _button*); /* Version without self-reference mechanism */
};


/* The Button type. Inherits from the Control type */
struct _button {
	struct _control *base; /* the base type */

	CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */

	/* ---------------------INHERITED------------------------------------------------------- */

	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */
	HINSTANCE *moduleInstance; /* The current module instance */
	char **className; /* The name of the window/control's WinAPI "class" */
	HMENU *ID; /* The child-window/control identifier */
	DWORD *styles; /* The window/control styles bitfield */
	DWORD *exStyles; /* The window/control extended styles bitfield */

	enum _objectType *type; /* The window/control type */

	/* event handlers */
	struct _event **events;
	unsigned int *numEvents;
	BOOL (*setEvent)(DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);
	BOOL (*setEventT)(struct _guiobject*, DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);

	BOOL (*setText)(char *text);
	BOOL (*setTextT)(struct _guiobject*, char *text);
	
	/* properties */
	char **text; /* The window/control text */

	int *width;
	int *height;

	int *x;
	int *y;

	BOOL *enabled;

	/* links */
	struct _guiobject **parent; /* A pointer to the parent */
	struct _guiobject ***children; /* A pointer to the array of children */
	unsigned int *numChildren; /* The number of children */

	/* methods */
	BOOL (*setPos)(int, int);
	BOOL (*setPosT)(struct _guiobject*, int, int);

	BOOL (*setSize)(int, int);
	BOOL (*setSizeT)(struct _guiobject*, int, int);

	BOOL (*addChild)(struct _control*); /* Add a child control to the control */
	BOOL (*addChildT)(struct _control*, struct _control*); /* Version without self-reference mechanism */

	BOOL (*addButton)(struct _button*); /* Add a button control to the control (for convenience) */
	BOOL (*addButtonT)(struct _control*, struct _button*); /* Version without self-reference mechanism */

	BOOL (*setEnabled)(BOOL);
	BOOL (*setEnabledT)(struct _guiobject*, BOOL);

	/* -----------------NOT INHERITED------------------------------------------------------- */
	BOOL (*setOnClick)(void(*)(struct _button*, void*, struct _eventArgs*), void*, enum _syncMode); /* Set an OnClick event for the control */
	BOOL (*setOnClickT)(struct _button*, void(*)(struct _button*, void*, struct _eventArgs*), void*, enum _syncMode); /* Version without self-reference mechanism */
};


/* The TextBox type. Inherits from the Control type */
struct _textbox {
	struct _control *base; /* the base type */

	CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */

	/* ---------------------INHERITED------------------------------------------------------- */

	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */
	HINSTANCE *moduleInstance; /* The current module instance */
	char **className; /* The name of the window/control's WinAPI "class" */
	HMENU *ID; /* The child-window/control identifier */
	DWORD *styles; /* The window/control styles bitfield */
	DWORD *exStyles; /* The window/control extended styles bitfield */

	enum _objectType *type; /* The window/control type */

	/* event handlers */
	struct _event **events;
	unsigned int *numEvents;
	BOOL (*setEvent)(DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);
	BOOL (*setEventT)(struct _guiobject*, DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);

	BOOL (*setText)(char *text);
	BOOL (*setTextT)(struct _guiobject*, char *text);
	
	/* properties */
	char **text; /* The window/control text */

	int *width;
	int *height;

	int *x;
	int *y;

	BOOL *enabled;

	/* links */
	struct _guiobject **parent; /* A pointer to the parent */
	struct _guiobject ***children; /* A pointer to the array of children */
	unsigned int *numChildren; /* The number of children */

	/* methods */
	BOOL (*setPos)(int, int);
	BOOL (*setPosT)(struct _guiobject*, int, int);

	BOOL (*setSize)(int, int);
	BOOL (*setSizeT)(struct _guiobject*, int, int);

	BOOL (*addChild)(struct _control*); /* Add a child control to the control */
	BOOL (*addChildT)(struct _control*, struct _control*); /* Version without self-reference mechanism */

	BOOL (*addButton)(struct _button*); /* Add a button control to the control (for convenience) */
	BOOL (*addButtonT)(struct _control*, struct _button*); /* Version without self-reference mechanism */

	BOOL (*setEnabled)(BOOL);
	BOOL (*setEnabledT)(struct _guiobject*, BOOL);

	/* -----------------NOT INHERITED------------------------------------------------------- */
	/* properties */
	BOOL *multiline;
	BOOL *numOnly;

	/* methods */
	BOOL (*setOnClick)(void(*)(struct _textbox*, void*, struct _eventArgs*), void*, enum _syncMode); /* Set an OnClick event for the control */
	BOOL (*setOnClickT)(struct _textbox*, void(*)(struct _textbox*, void*, struct _eventArgs*), void*, enum _syncMode); /* Version without self-reference mechanism */

	BOOL (*setNumOnly)(BOOL);
	BOOL (*setNumOnlyT)(struct _textbox*, BOOL);
};


/* The Label type. Inherits from the Control type */
struct _label {
	struct _control *base; /* the base type */

	CRITICAL_SECTION *criticalSection; /* The critical section for synchronising access to the object */
	LONG_PTR *origProcPtr; /* The pointer to the original window procedure */

	/* ---------------------INHERITED------------------------------------------------------- */

	HWND *handle; /* The handle to the window/control; initialized with a call to CreateWindowEx */
	HINSTANCE *moduleInstance; /* The current module instance */
	char **className; /* The name of the window/control's WinAPI "class" */
	HMENU *ID; /* The child-window/control identifier */
	DWORD *styles; /* The window/control styles bitfield */
	DWORD *exStyles; /* The window/control extended styles bitfield */

	enum _objectType *type; /* The window/control type */

	/* event handlers */
	struct _event **events;
	unsigned int *numEvents;
	BOOL (*setEvent)(DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);
	BOOL (*setEventT)(struct _guiobject*, DWORD, void(*)(void*, void*, struct _eventArgs*), void*, enum _syncMode);

	BOOL (*setText)(char *text);
	BOOL (*setTextT)(struct _guiobject*, char *text);
	
	/* properties */
	char **text; /* The window/control text */

	int *width;
	int *height;

	int *x;
	int *y;

	BOOL *enabled;

	/* links */
	struct _guiobject **parent; /* A pointer to the parent */
	struct _guiobject ***children; /* A pointer to the array of children */
	unsigned int *numChildren; /* The number of children */

	/* methods */
	BOOL (*setPos)(int, int);
	BOOL (*setPosT)(struct _guiobject*, int, int);

	BOOL (*setSize)(int, int);
	BOOL (*setSizeT)(struct _guiobject*, int, int);

	BOOL (*addChild)(struct _control*); /* Add a child control to the control */
	BOOL (*addChildT)(struct _control*, struct _control*); /* Version without self-reference mechanism */

	BOOL (*addButton)(struct _button*); /* Add a button control to the control (for convenience) */
	BOOL (*addButtonT)(struct _control*, struct _button*); /* Version without self-reference mechanism */

	BOOL (*setEnabled)(BOOL);
	BOOL (*setEnabledT)(struct _guiobject*, BOOL);

	/* -----------------NOT INHERITED------------------------------------------------------- */
	/* properties */
	BOOL *multiline;
	BOOL *numOnly;

	/* methods */
	BOOL (*setOnClick)(void(*)(struct _textbox*, void*, struct _eventArgs*), void*, enum _syncMode); /* Set an OnClick event for the control */
	BOOL (*setOnClickT)(struct _textbox*, void(*)(struct _textbox*, void*, struct _eventArgs*), void*, enum _syncMode); /* Version without self-reference mechanism */
};


/* The current thread context */
struct _threadContext {
	struct _guiobject **threadObjects; /* A pointer to a list of all objects existing in the current thread */
	unsigned int numObjects; /* The number of objects in the current thread */

	void *currentThis; /* A pointer to the instance of the object currently being used in this thread */
	DWORD threadID; /* The current thread's ID */
};

/* =============================================TYPEDEFS=================================================================== */

typedef struct _guiobject *GUIObject;
typedef struct _window *Window;
typedef struct _control *Control;
typedef struct _button *Button;
typedef struct _textbox *TextBox;
typedef struct _label *Label;

typedef struct _eventArgs *EventArgs;
typedef struct _eventArgs *MouseEventArgs; //TODO!

typedef void(*Callback)(void*, void*, struct _eventArgs*);
typedef void(*ButtonCallback)(struct _button*, void*, struct _eventArgs*);

/* ==========================NON-ENCAPSULATED FUNCTION PROTOTYPES========================================================== */

void setCurrentThis(void *self);

/* Constructors */
struct _guiobject *newObject(HINSTANCE instance, char *caption, int width, int height);
struct _window *newWindow(HINSTANCE instance, char *caption, int width, int height);
struct _control *newControl(HINSTANCE instance, char *caption, int width, int height, int x, int y);
struct _button *newButton(HINSTANCE instance, char *caption, int width, int height, int x, int y);
struct _textbox *newTextBox(HINSTANCE instance, char *caption, int width, int height, int x, int y, enum _textboxtype multiline);
struct _label *newLabel(HINSTANCE instance, char *caption, int width, int height, int x, int y);

/* Desctructors */
BOOL deleteObject(struct _guiobject *object);
BOOL deleteWindow(struct _window *window);
BOOL deleteControl(struct _control *control);
BOOL deleteButton(struct _button *button);
BOOL deleteTextBox(struct _textbox *textbox);
BOOL deleteLabel(struct _label *label);

BOOL displayWindow(struct _window *mainWindow, int nCmdShow);

/* =============================================MACROS===================================================================== */

/* "Method" call */
#define $(object) setCurrentThis((void*)object), object

/* Methods for adding children of types inherited from Control */
#define addButton(button) addChild(button->base)
#define addTextBox(textbox) addChild(textbox->base)
#define addLabel(label) addChild(label->base)

/* "Cast" object to base type */
#define castToBase(object) object->base

/* "Cast" the base type of an object back to derived type, or return NULL if this object is of the base type. Use with caution! */
#define castGUIObjectToWindow(object) *((struct _guiobject*)object)->type == WINDOW ? \
										(struct _window*)(((struct _guiobject*)object)->derived) : NULL;
#define castGUIObjectToControl(object) *((struct _guiobject*)object)->type != WINDOW ? \
										(struct _control*)(((struct _guiobject*)object)->derived) : NULL;
#define castGUIObjectToButton(object) *((struct _guiobject*)object)->type == BUTTON ? \
										((struct _control*)(((struct _guiobject*)object)->derived) ? \
										(struct _button*)((struct _control*)(((struct _guiobject*)object)->derived)->derived) : NULL) : NULL;
#define castControlToButton(object) *((struct _control*)object)->type == BUTTON ? \
										(struct _button*)(((struct _control*)object)->derived) : NULL;

/* Synchronize access to current object */
#define startSync(object) EnterCriticalSection(object->criticalSection)
#define endSync(object) LeaveCriticalSection(object->criticalSection)
