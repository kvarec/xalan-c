/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 1999-2004 The Apache Software Foundation.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in
 *	  the documentation and/or other materials provided with the
 *	  distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *	  if any, must include the following acknowledgment:  
 *		 "This product includes software developed by the
 *		  Apache Software Foundation (http://www.apache.org/)."
 *	  Alternately, this acknowledgment may appear in the software itself,
 *	  if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xalan" and "Apache Software Foundation" must
 *	  not be used to endorse or promote products derived from this
 *	  software without prior written permission. For written 
 *	  permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *	  nor may "Apache" appear in their name, without prior written
 *	  permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.	IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */
#include "XercesParserLiaison.hpp"



#if defined(XALAN_CLASSIC_IOSTREAMS)
#include <iostream.h>
#else
#include <iostream>
#endif



#include <xercesc/framework/URLInputSource.hpp>
#if XERCES_VERSION_MAJOR >= 2
#include <xercesc/parsers/XercesDOMParser.hpp>
#else
#include <xercesc/parsers/DOMParser.hpp>
#endif
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/SAXParseException.hpp>



#include <xalanc/Include/XalanAutoPtr.hpp>
#include <xalanc/Include/STLHelper.hpp>



#include <xalanc/PlatformSupport/ExecutionContext.hpp>
#include <xalanc/PlatformSupport/XalanMessageLoader.hpp>
#include <xalanc/PlatformSupport/XalanUnicode.hpp>



#include <xalanc/DOMSupport/DOMSupport.hpp>



#include <xalanc/XercesParserLiaison/Deprecated/XercesDocumentBridge.hpp>
#include <xalanc/XercesParserLiaison/XercesDocumentWrapper.hpp>
#include <xalanc/XercesParserLiaison/XercesDOMSupport.hpp>



XALAN_CPP_NAMESPACE_BEGIN



XercesParserLiaison::XercesParserLiaison(XercesDOMSupport&	/* theSupport */) :
	m_indent(-1),
	m_useValidation(false),
	m_includeIgnorableWhitespace(true),
	m_doNamespaces(true),
	m_exitOnFirstFatalError(true),
	m_entityResolver(0),
	m_errorHandler(this),
	m_externalSchemaLocation(),
	m_externalNoNamespaceSchemaLocation(),
	m_documentMap(),
	m_buildBridge(true),
	m_threadSafe(false),
	m_buildMaps(false),
	m_executionContext(0)
{
}



XercesParserLiaison::XercesParserLiaison() :
	m_indent(-1),
	m_useValidation(false),
	m_includeIgnorableWhitespace(true),
	m_doNamespaces(true),
	m_exitOnFirstFatalError(true),
	m_entityResolver(0),
	m_errorHandler(this),
	m_externalSchemaLocation(),
	m_externalNoNamespaceSchemaLocation(),
	m_documentMap(),
	m_buildWrapper(true),
	m_buildBridge(true),
	m_threadSafe(false),
	m_buildMaps(false),
	m_executionContext(0)
{
}



XercesParserLiaison::~XercesParserLiaison()
{
	reset();
}



void
XercesParserLiaison::reset()
{
	// Delete any live documents...
	for(DocumentMapType::iterator i = m_documentMap.begin();
		i != m_documentMap.end();
		++i)
	{
		if ((*i).second.m_isDeprecated == false &&
			(*i).second.m_isOwned == true)
		{
#if defined(XALAN_CANNOT_DELETE_CONST)
			delete (DOMDocument_Type*)(*i).second.m_wrapper->getXercesDocument();
#else
			delete (*i).second.m_wrapper->getXercesDocument();
#endif
		}

#if defined(XALAN_CANNOT_DELETE_CONST)
		delete (XalanDocument*)(*i).first;
#else
		delete (*i).first;
#endif
	}

	m_documentMap.clear();

	m_executionContext = 0;
}



ExecutionContext*
XercesParserLiaison::getExecutionContext() const
{
	return m_executionContext;
}



void
XercesParserLiaison::setExecutionContext(ExecutionContext&	theContext)
{
	m_executionContext = &theContext;
}



void
XercesParserLiaison::parseXMLStream(
			const InputSourceType&	urlInputSource,
			DocumentHandlerType&	handler,
			const XalanDOMString&	/* identifier */)
{
	XalanAutoPtr<SAXParserType> 	theParser(CreateSAXParser());

	theParser->setDocumentHandler(&handler);

	if (m_errorHandler == 0)
	{
		theParser->setErrorHandler(this);
	}
	else
	{
		theParser->setErrorHandler(m_errorHandler);
	}

	theParser->parse(urlInputSource);
}



XalanDocument*
XercesParserLiaison::parseXMLStream(
			const InputSourceType&	reader,
			const XalanDOMString&	/* identifier */)
{
	XalanAutoPtr<DOMParserType> 		theParser(CreateDOMParser());

	if (m_errorHandler == 0)
	{
		theParser->setErrorHandler(this);
	}
	else
	{
		theParser->setErrorHandler(m_errorHandler);
	}

	theParser->parse(reader);

#if XERCES_VERSION_MAJOR >= 2
	DOMDocument_Type* const	theXercesDocument =
		theParser->getDocument();

	theXercesDocument->normalize();
#else
	DOM_Document_Type	theXercesDocument =
		theParser->getDocument();

	theXercesDocument.normalize();
#endif

#if XERCES_VERSION_MAJOR >= 2
	XercesDocumentWrapper*	theNewDocument = 0;

	if (theXercesDocument != 0)
	{
		theNewDocument = doCreateDocument(theXercesDocument, m_threadSafe, m_buildWrapper, m_buildMaps, true);

		theParser->adoptDocument();
#else
	XercesDocumentBridge*	theNewDocument = 0;

	if (theXercesDocument.isNull() == false)
	{
		theNewDocument = doCreateDocument(theXercesDocument, m_threadSafe, m_buildBridge, true);
#endif
	}

	return theNewDocument;
}



XalanDocument*
XercesParserLiaison::createDocument()
{
	const DOM_Document_Type	theXercesDocument =
		DOM_Document_Type::createDocument();

	return createDocument(theXercesDocument, false, false);
}



XalanDocument*
XercesParserLiaison::createDOMFactory()
{
	return createDocument();
}



void
XercesParserLiaison::destroyDocument(XalanDocument* 	theDocument)
{
	const DocumentMapType::iterator		i =
		m_documentMap.find(theDocument);

	if (i != m_documentMap.end())
	{
		const XalanAutoPtr<XalanDocument>	theGuard(theDocument);

		m_documentMap.erase(i);
	}
}



int
XercesParserLiaison::getIndent() const
{
	return m_indent;
}



void
XercesParserLiaison::setIndent(int	i)
{
	m_indent = i;
}



bool
XercesParserLiaison::getUseValidation() const
{
	return m_useValidation;
}



void
XercesParserLiaison::setUseValidation(bool	b)
{
	m_useValidation = b;
}



const XalanDOMString
XercesParserLiaison::getParserDescription() const
{
	return StaticStringToDOMString(XALAN_STATIC_UCODE_STRING("Xerces"));
}



bool
XercesParserLiaison::getIncludeIgnorableWhitespace() const
{
	return m_includeIgnorableWhitespace;
}



void
XercesParserLiaison::setIncludeIgnorableWhitespace(bool include)
{
	m_includeIgnorableWhitespace = include;
}



ErrorHandlerType*
XercesParserLiaison::getErrorHandler() const
{
	return m_errorHandler;
}



void
XercesParserLiaison::setErrorHandler(ErrorHandlerType*	handler)
{
	m_errorHandler = handler;
}



bool
XercesParserLiaison::getDoNamespaces() const
{
	return m_doNamespaces;
}



void
XercesParserLiaison::setDoNamespaces(bool	newState)
{
	m_doNamespaces = newState;
}



bool
XercesParserLiaison::getExitOnFirstFatalError() const
{
	return m_exitOnFirstFatalError;
}



void
XercesParserLiaison::setExitOnFirstFatalError(bool	newState)
{
	m_exitOnFirstFatalError = newState;
}



EntityResolverType*
XercesParserLiaison::getEntityResolver() const
{
	return m_entityResolver;
}



void
XercesParserLiaison::setEntityResolver(EntityResolverType*	resolver)
{
	m_entityResolver = resolver;
}



const XalanDOMChar*
XercesParserLiaison::getExternalSchemaLocation() const
{
	return m_externalSchemaLocation.length() == 0 ? 0 : m_externalSchemaLocation.c_str();
}



void
XercesParserLiaison::setExternalSchemaLocation(const XalanDOMChar*	location)
{
	if (location == 0)
	{
		m_externalSchemaLocation.clear();
	}
	else
	{
		m_externalSchemaLocation = location;
	}
}



const XalanDOMChar*
XercesParserLiaison::getExternalNoNamespaceSchemaLocation() const
{
	return m_externalNoNamespaceSchemaLocation.length() == 0 ? 0 : m_externalNoNamespaceSchemaLocation.c_str();
}



void
XercesParserLiaison::setExternalNoNamespaceSchemaLocation(const XalanDOMChar*	location)
{
	if (location == 0)
	{
		m_externalNoNamespaceSchemaLocation.clear();
	}
	else
	{
		m_externalNoNamespaceSchemaLocation= location;
	}
}



XalanDocument*
XercesParserLiaison::createDocument(
			const DOM_Document_Type& 	theXercesDocument,
			bool						threadSafe,
			bool						buildBridge)
{
	return doCreateDocument(theXercesDocument, threadSafe, buildBridge);
}



XalanDocument*
XercesParserLiaison::createDocument(
			const DOMDocument_Type*		theXercesDocument,
			bool						threadSafe,
			bool						buildWrapper,
			bool						buildMaps)
{
	// As we did not create the underlying DOMDocument - ensure we don't
	// delete it later.
	return doCreateDocument(theXercesDocument, threadSafe, buildWrapper, buildMaps, false);
}



XercesDocumentBridge*
XercesParserLiaison::mapDocument(const XalanDocument*	theDocument) const
{
	const DocumentMapType::const_iterator	i =
		m_documentMap.find(theDocument);

	return i != m_documentMap.end() ? (*i).second.m_isDeprecated == true ? (*i).second.m_bridge : 0 : 0;
}



XercesDocumentWrapper*
XercesParserLiaison::mapDocumentToWrapper(const XalanDocument*	theDocument) const
{
	const DocumentMapType::const_iterator	i =
		m_documentMap.find(theDocument);

	return i != m_documentMap.end() ? (*i).second.m_isDeprecated == false ? (*i).second.m_wrapper : 0 : 0;
}



DOM_Document_Type
XercesParserLiaison::mapXercesDocument(const XalanDocument* 	theDocument) const
{
	const DocumentMapType::const_iterator	i =
		m_documentMap.find(theDocument);

	return i != m_documentMap.end() ? (*i).second.m_isDeprecated == true ? (*i).second.m_bridge->getXercesDocument() : DOM_Document_Type() : DOM_Document_Type();
}



const DOMDocument_Type*
XercesParserLiaison::mapToXercesDocument(const XalanDocument*	theDocument) const
{
	const DocumentMapType::const_iterator	i =
		m_documentMap.find(theDocument);

	return i != m_documentMap.end() ? (*i).second.m_isDeprecated == false ? (*i).second.m_wrapper->getXercesDocument() : 0 : 0;
}



void
XercesParserLiaison::fatalError(const SAXParseExceptionType&	e)
{
	XalanDOMString	theMessage = XalanMessageLoader::getMessage(XalanMessages::FatalError);

	formatErrorMessage(e, theMessage);

	if (m_executionContext != 0)
	{
		// We call warning() because we don't want the execution
		// context to potentially throw an exception.
		m_executionContext->warn(theMessage);
	}
	else
	{
		XALAN_USING_STD(cerr)
		XALAN_USING_STD(endl)

		cerr << endl << theMessage << endl;
	}

	throw e;
}



void
XercesParserLiaison::error(const SAXParseExceptionType& 	e)
{
	XalanDOMString	theMessage = XalanMessageLoader::getMessage(XalanMessages::Error2);

	formatErrorMessage(e, theMessage);

	if (m_executionContext != 0)
	{
		// We call warn() because we don't want the execution
		// context to potentially throw an exception.
		m_executionContext->warn(theMessage);
	}
	else
	{
		XALAN_USING_STD(cerr)
		XALAN_USING_STD(endl)

		cerr << endl << theMessage << endl;
	}

	if (m_useValidation == true)
	{
		throw e;
	}
}



void
XercesParserLiaison::warning(const SAXParseExceptionType&	e)
{
	XalanDOMString	theMessage = XalanMessageLoader::getMessage(XalanMessages::Warning2);

	formatErrorMessage(e, theMessage);

	if (m_executionContext != 0)
	{
		m_executionContext->warn(theMessage);
	}
	else
	{
		XALAN_USING_STD(cerr)
		XALAN_USING_STD(endl)

		cerr << endl << theMessage << endl;
	}
}



void
XercesParserLiaison::formatErrorMessage(
			const SAXParseExceptionType&	e,
			XalanDOMString& 				theMessage)
{

	const XalanDOMChar* const	theSystemID = e.getSystemId();

	if (theSystemID == 0 || length(theSystemID) == 0)
	{

		append(theMessage,XalanMessageLoader::getMessage(XalanMessages::AtUnknownFileLineColumn_2Param
			,LongToDOMString(long(e.getLineNumber()))
			,LongToDOMString(long(e.getColumnNumber()))
			));
	}
	else
	{
		append(theMessage,XalanMessageLoader::getMessage(XalanMessages::AtFileLineColumn_3Param
			,XalanDOMString(theSystemID)
			,LongToDOMString(long(e.getLineNumber()))
			,LongToDOMString(long(e.getColumnNumber()))
			));
	}

	append(theMessage, e.getMessage());
}



void
XercesParserLiaison::resetErrors()
{
}



XercesParserLiaison::DOMParserType*
XercesParserLiaison::CreateDOMParser()
{
	DOMParserType* const	theParser = new DOMParserType;

	theParser->setExpandEntityReferences(true);

	theParser->setValidationScheme(m_useValidation == true ? DOMParserType::Val_Auto : DOMParserType::Val_Never);

	theParser->setIncludeIgnorableWhitespace(m_includeIgnorableWhitespace);

	theParser->setDoNamespaces(m_doNamespaces);

	theParser->setExitOnFirstFatalError(m_exitOnFirstFatalError);

	if (m_entityResolver != 0)
	{
		theParser->setEntityResolver(m_entityResolver);
	}

	theParser->setErrorHandler(m_errorHandler);

	if (m_externalSchemaLocation.length() > 0)
	{
		theParser->setExternalSchemaLocation(c_wstr(m_externalSchemaLocation));
	}

	if (m_externalNoNamespaceSchemaLocation.length() > 0)
	{
		theParser->setExternalNoNamespaceSchemaLocation(c_wstr(m_externalNoNamespaceSchemaLocation));
	}

#if XERCES_VERSION_MAJOR < 2
	// Xerces has a non-standard node type to represent the XML decl.
	// Why did they ever do this?
	theParser->setToCreateXMLDeclTypeNode(false);
#endif

	return theParser;
}



XercesParserLiaison::SAXParserType*
XercesParserLiaison::CreateSAXParser()
{
	SAXParserType* const	theParser = new SAXParserType;

	theParser->setDoValidation(false);

	theParser->setDoNamespaces(false);

	theParser->setExitOnFirstFatalError(m_exitOnFirstFatalError);

	if (m_entityResolver != 0)
	{
		theParser->setEntityResolver(m_entityResolver);
	}

	theParser->setErrorHandler(m_errorHandler);

	return theParser;
}



XercesDocumentBridge*
XercesParserLiaison::doCreateDocument(
			const DOM_Document_Type& 	theXercesDocument,
			bool						threadSafe,
			bool						buildBridge)
{
	XercesDocumentBridge* const 	theNewDocument =
		new XercesDocumentBridge(theXercesDocument, threadSafe, buildBridge);

	m_documentMap[theNewDocument] = theNewDocument;

	return theNewDocument;
}



XercesDocumentWrapper*
XercesParserLiaison::doCreateDocument(
			const DOMDocument_Type*		theXercesDocument,
			bool						threadSafe,
			bool						buildWrapper,
			bool						buildMaps,
			bool						isOwned)
{
	XercesDocumentWrapper* const		theNewDocument =
		new XercesDocumentWrapper(theXercesDocument, threadSafe, buildWrapper, buildMaps);

	DocumentEntry&	theEntry = m_documentMap[theNewDocument];
	
	theEntry = theNewDocument;
	theEntry.m_isOwned = isOwned;

	return theNewDocument;
}



XALAN_CPP_NAMESPACE_END
