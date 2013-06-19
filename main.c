#include <stdio.h>
#include <libxml/xmlerror.h>
#include <libxml/parser.h>
#include <libxml/SAX2.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

// http://ichiba-blog.blogspot.jp/2011/01/libxml2rss.html

#define DUMPPOSTION 1

#define kStringLengthMax (65535)

#define _TRACE( v, l ) traceXmlChar( #v, v, l );

void traceXmlChar( const char const* name, const xmlChar *var, int level );

static void startElementCallback (
								  void *ctx,
								  const xmlChar *localname,
								  const xmlChar *prefix,
								  const xmlChar *URI,
								  int nb_namespaces,
								  const xmlChar **namespaces,
								  int nb_attributes,
								  int nb_defaulted,
								  const xmlChar **attributes
								  );
static void endElementCallback (
								void *ctx,
								const xmlChar *localname,
								const xmlChar *prefix,
								const xmlChar *URI);
static void charactersCallback (
								void *ctx,
								const xmlChar *ch,
								int len);

static xmlSAXHandler gSaxHandler = {
    NULL,            /* internalSubset */
    NULL,            /* isStandalone   */
    NULL,            /* hasInternalSubset */
    NULL,            /* hasExternalSubset */
    NULL,            /* resolveEntity */
    NULL,            /* getEntity */
    NULL,            /* entityDecl */
    NULL,            /* notationDecl */
    NULL,            /* attributeDecl */
    NULL,            /* elementDecl */
    NULL,            /* unparsedEntityDecl */
    NULL,            /* setDocumentLocator */
    NULL,            /* startDocument */
    NULL,            /* endDocument */
    NULL,            /* startElement*/
    NULL,            /* endElement */
    NULL,            /* reference */
    charactersCallback, /* characters */
    NULL,            /* ignorableWhitespace */
    NULL,            /* processingInstruction */
    NULL,            /* comment */
    NULL,            /* warning */
    NULL,            /* error */
    NULL,            /* fatalError //: unused error() get all the errors */
    NULL,            /* getParameterEntity */
    NULL,            /* cdataBlock */
    NULL,            /* externalSubset */
    XML_SAX2_MAGIC,  /* initialized */
    NULL,            /* private */
    startElementCallback,    /* startElementNs */
    endElementCallback,      /* endElementNs */
    NULL,            /* serror */
};

int inData;
int countData;

int main (int argc, const char * argv[]) {
	inData = 0;
	countData = 0;
	httpreq();
    return 0;
}

// http://developer.apple.com/library/ios/#documentation/Networking/Conceptual/CFNetwork/CFHTTPTasks/CFHTTPTasks.html
// http://oreilly.com/iphone/excerpts/iphone-sdk/network-programming.html

// http://cryptofreek.org/2013/04/15/asynchronous-http-request-using-cfnetwork-framework/

CFMutableDataRef dataBuffer;

#if USERRUNLOOP
void _handleFinishCondition( CFReadStreamRef stream, bool bDeleteDownloadedFile, int nExitCode )
{
	CFReadStreamUnscheduleFromRunLoop( stream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes );
	CFReadStreamClose(stream);
	CFRelease(stream);
	
	CFRunLoopStop(CFRunLoopGetCurrent()); 
	//	exit(nExitCode);
}

void readCallBack( CFReadStreamRef stream, CFStreamEventType event, void* ptr )
{
	switch( event )
	{
		case kCFStreamEventHasBytesAvailable:
		{
			UInt8 buff[1024];
			CFIndex nBytesRead = CFReadStreamRead(stream, buff, 1024);
			
			if( nBytesRead>0 )
			{
				//				file.write( (const char*)buff, nBytesRead );
				CFDataAppendBytes(dataBuffer, buff, nBytesRead);
//				printf("read %d bytes\n", nBytesRead);
			}
			
			break;
		}
		case kCFStreamEventErrorOccurred:
		{
			CFStreamError err = CFReadStreamGetError(stream);
			_handleFinishCondition(stream, true, err.error);
		}
		case kCFStreamEventEndEncountered:
		{
			_handleFinishCondition(stream, false, 0);
		}
	}
}
#endif

int httpreq()
{
	CFStringRef bodyString = CFSTR(""); // Usually used for POST data
	CFDataRef bodyData = CFStringCreateExternalRepresentation(kCFAllocatorDefault,
															  bodyString, kCFStringEncodingUTF8, 0);
	
	CFStreamClientContext CTX = { 0, NULL, NULL, NULL, NULL };
	
	CFStringRef headerFieldName = CFSTR("X-My-Favorite-Field");
	CFStringRef headerFieldValue = CFSTR("Dreams");
	
	CFStringRef url = CFSTR("http://www.interfm.co.jp/jazz/blog/feed/");
	CFURLRef myURL = CFURLCreateWithString(kCFAllocatorDefault, url, NULL);
	
	CFStringRef requestMethod = CFSTR("GET");
	CFHTTPMessageRef myRequest =
	CFHTTPMessageCreateRequest(kCFAllocatorDefault, requestMethod, myURL,
                               kCFHTTPVersion1_1);
	
	//	CFDataRef bodyDataExt = CFStringCreateExternalRepresentation(kCFAllocatorDefault, bodyData, kCFStringEncodingUTF8, 0);
	//	CFHTTPMessageSetBody(myRequest, bodyDataExt);
	//	CFStringRef requestMessage = CFSTR("");
	//	CFHTTPMessageSetBody(myRequest, (CFDataRef) requestMessage);
	
	CFHTTPMessageSetHeaderFieldValue(myRequest, headerFieldName, headerFieldValue);
	CFDataRef mySerializedRequest = CFHTTPMessageCopySerializedMessage(myRequest);
//	printf("%.*s", (int)CFDataGetLength(mySerializedRequest), CFDataGetBytePtr(mySerializedRequest));
	
	CFReadStreamRef myReadStream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, myRequest);

#if USERRUNLOOP
	if (!CFReadStreamSetClient(myReadStream, kCFStreamEventOpenCompleted |
							   kCFStreamEventHasBytesAvailable |
							   kCFStreamEventEndEncountered |
							   kCFStreamEventErrorOccurred,
							   readCallBack, &CTX))
    {
        CFRelease(myReadStream);
		printf("CFReadStreamSetClient error\n");
        return 0;
	}
#endif
	dataBuffer = CFDataCreateMutable(kCFAllocatorDefault, 0);
	
	/* Add to the run loop */
    CFReadStreamScheduleWithRunLoop(myReadStream, CFRunLoopGetCurrent(),
									kCFRunLoopCommonModes);
	
	CFReadStreamOpen( myReadStream );
	
#if USERRUNLOOP
	CFRunLoopRun();
#else
	CFIndex nBytesRead;
	do {
		UInt8 buff[1024]; // define myReadBufferSize as desired
		nBytesRead = CFReadStreamRead(myReadStream, buff, 1024);
		if( nBytesRead > 0 ) {
			CFDataAppendBytes(dataBuffer, buff, nBytesRead);
		} else if( nBytesRead < 0 ) {
			CFStreamError error = CFReadStreamGetError(myReadStream);
			printf("error %d\n",error);
			return 0;
		}
	} while( nBytesRead > 0 );
#endif
//	printf("exit runloop %d\n", CFDataGetLength(dataBuffer));
	CFHTTPMessageRef myResponse = (CFHTTPMessageRef)CFReadStreamCopyProperty(myReadStream,
																			 kCFStreamPropertyHTTPResponseHeader);
	CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine(myResponse);
	UInt32 statusCode = CFHTTPMessageGetResponseStatusCode(myResponse);
//	printf("status code %d\n",statusCode);
	
	if(statusCode == 200) {
		CFStringRef contentStrRef = CFHTTPMessageCopyHeaderFieldValue(myResponse, CFSTR("Content-Type"));
		char hdrbuff[1024];
		CFStringGetCString( contentStrRef, hdrbuff, 1024, kCFStringEncodingUTF8 );
//		printf("content-type %s\n", hdrbuff);
		if(strcmp("text/xml; charset=UTF-8", hdrbuff) == 0) {
			xmlParserCtxtPtr parserContext;
			parserContext = xmlCreatePushParserCtxt(&gSaxHandler, NULL, NULL, 0, NULL);
		
			xmlParseChunk( parserContext, (const char*)CFDataGetBytePtr(dataBuffer), CFDataGetLength(dataBuffer), 0);
		}
	}

	CFRelease(myRequest);
	CFRelease(myURL);
	CFRelease(url);
	CFRelease(mySerializedRequest);
	myRequest = NULL;
	mySerializedRequest = NULL;
	
	return 0;
}

static void startElementCallback (
								  void *ctx,
								  const xmlChar *localname,
								  const xmlChar *prefix,
								  const xmlChar *URI,
								  int nb_namespaces,
								  const xmlChar **namespaces,
								  int nb_attributes,
								  int nb_defaulted,
								  const xmlChar **attributes ) {
//	printf( "startElementCallback\n" );
//	_TRACE( localname, 1 );
//	_TRACE( prefix, 1 );
//	_TRACE( URI, 1 );
	if(xmlStrcmp(localname, (const xmlChar *)"encoded") == 0 && 
	   xmlStrcmp(prefix, (const xmlChar *)"content") == 0) {
		++countData;
		if(countData == DUMPPOSTION) {
			inData = 1;
		}
	}
	
	int i;
	for ( i = 0; i < nb_namespaces; i++ ) {
//		_TRACE( namespaces[i], 2 );
	}
	
	for ( i = 0; i < nb_attributes; i++ ) {
		char tmp[ kStringLengthMax ];
		// http://stackoverflow.com/questions/2075894/how-to-get-the-name-and-value-of-attributes-from-xml-when-using-libxml2-sax-parse
		int len = attributes[4] - attributes[3]; // attributes[3]のデータ長計算
		memcpy( tmp, attributes[3], len );
		tmp[len] = '\0'; // attributes[3]をC文字列として扱う
//		printf( "\t\tattributes[%d] - key:%s, value:%s\n", i, attributes[0], tmp );
		attributes += 5;
	}
}

static void endElementCallback (
								void *ctx,
								const xmlChar *localname,
								const xmlChar *prefix,
								const xmlChar *URI) {
//	printf( "endElementCallback\n" );
	if(xmlStrcmp(localname, (const xmlChar *)"encoded") == 0 && 
	   xmlStrcmp(prefix, (const xmlChar *)"content") == 0) {
		inData = 0;
	}
	
//	_TRACE( localname, 1 );
//	_TRACE( prefix, 1 );
//	_TRACE( URI, 1 );
}

static void charactersCallback (
								void *ctx,
								const xmlChar *ch,
								int len) {
	char buf[ kStringLengthMax ];
//	printf( "charactersCallback\n" );
	if ( NULL != ch ) {
//		memcpy( buf, ch, len );
		char *ptr = ch;
		int cplen = 0;
		int i;
		int intag = 0;
		for(i = 0; i < len ; ++i) {
			if(*ptr == '<') {
				intag = 1;
			}
			if(intag == 0) {
				if(*ptr == '&') {
					if(len - i >= 5 && strncmp(ptr, "&amp;", 5) == 0) {
						buf[cplen++] = '&';
						i += 4;
						ptr += 4;
					} else if(len - i >= 4 && strncmp(ptr, "&gt;", 4) == 0) {
						buf[cplen++] = '<';
						i += 3;
						ptr += 3;
					} else if(len - i >= 4 && strncmp(ptr, "&lt;", 4) == 0) {
						buf[cplen++] = '>';
						i += 3;
						ptr += 3;
					} else if(len - i >= 6 && strncmp(ptr, "&quot;", 6) == 0) {
						buf[cplen++] = '"';
						i += 5;
						ptr += 5;
					} else if(len - i >= 6 && strncmp(ptr, "&nbsp;", 6) == 0) {
						i += 5;
						ptr += 5;
					} else {  // this is unsupport escape
						buf[cplen++] = *ptr;
					}
				} else {
					if(cplen != 0 && buf[cplen-1] == '\n' && *ptr == '\n') {
						// delete duplicate line feed
					} else {
						buf[cplen++] = *ptr;
					}
				}
			}
			if(*ptr == '>') {
				intag = 0;
			}
			++ptr;
		}
		buf[cplen] = '\0';
		if(inData == 1) {
			printf( "%s\n", buf); 
		}
	}
}

void traceXmlChar( const char const* name, const xmlChar *var, int level ) {
	int i;
	if ( NULL != var ) { 
		for ( i = 0; i < level; i++ ) {
			putchar( '\t' );
		}
		printf( "%s:%s\n", name, var );
	}
}