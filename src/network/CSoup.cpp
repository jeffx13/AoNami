//#include "htmlparser.h"
//#include <iostream>
#include "CSoup.h"


std::string CSoup::tidy(const char *input){
    TidyBuffer output = {0};
    TidyBuffer errbuf = {0};
    int rc = -1;
    Bool ok;

    TidyDoc tdoc = tidyCreate();                     // Initialize "document"

    ok = tidyOptSetBool( tdoc, TidyXhtmlOut, yes );  // Convert to XHTML
    if ( ok )
        rc = tidySetErrorBuffer( tdoc, &errbuf );      // Capture diagnostics
    if ( rc >= 0 )
        rc = tidyParseString( tdoc, input);           // Parse the input
    if ( rc >= 0 )
        rc = tidyCleanAndRepair( tdoc );               // Tidy it up!
    if ( rc >= 0 )
        rc = tidyRunDiagnostics( tdoc );               // Kvetch
    if ( rc > 1 )                                    // If error, force output.
        rc = ( tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1 );
    if ( rc >= 0 )
        rc = tidySaveBuffer( tdoc, &output );          // Pretty Print

    if ( rc >= 0 )
    {
        tidyBufFree( &errbuf );
        tidyRelease( tdoc );
        int len = strlen(reinterpret_cast<const char*>(output.bp));
        std::string str2(output.bp, output.bp + len);
        tidyBufFree( &output );
        return str2;
    }
    else
    {
        std::string error = "Tidy error: ";
        error += reinterpret_cast<const char*>(errbuf.bp);
        tidyBufFree( &errbuf );
        tidyRelease( tdoc );
        throw std::runtime_error(error);
    }
}

bool CSoup::parse(const char *source){
    pugi::xml_parse_result parsed = doc.load_string(source);
    if (parsed)
    {
        //            std::cout << "XML [" << source << "] parsed without errors, attr value: [" << doc.child("html").attribute("xmlns").value() << "]\n\n";
        return true;
    }
    else
    {
        std::cout << "XML [" << source << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
        std::cout << "Error description: " << parsed.description() << "\n";
        std::cout << "Error offset: " << parsed.offset << " (error at [..." << (source + parsed.offset) << "]\n\n";
    }
    return false;
}
