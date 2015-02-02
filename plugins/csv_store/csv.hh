//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
// Generic logging device.
//
#ifndef __CSV__
#define __CSV__

#include "component.hh"
class CCSVField: public CExecutable {
public:
    XOBJECT_TYPE(CCSVField, 
		 "CSVField", 
		 "CSV Field descriptor",
		 (CCSVField_name,
		  CCSVField_format,
		  CCSVField_data),

		 XFIELD(CCSVField, Q_PUBLIC, name, 
			string_type(),
			"Name of field"),
		 XFIELD(CCSVField, Q_PUBLIC, format,
			string_type(),
			"printf format string [%f]"),
		 XFIELD(CCSVField,Q_PUBLIC, data,
			CArrayType::create(float_type(),0),
			"Floating point data")
	);
public:
    CCSVField(CExecutor* aExec, CBaseType *aType = CCSVFieldType::singleton()):
	CExecutable(aExec, aType) {
	put(aExec, XINDEX(CCSVField,format), UString(m1New(CString, "%f")));
    }

    ~CCSVField(void) {
	m1ReleaseArray(data());
    }
    void execute(CExecutor* aExec) {};

    const char *name(void)   { return at(XINDEX(CCSVField,name)).str->c_str(); }
    const char *format(void) { return at(XINDEX(CCSVField,format)).str->c_str(); }
    CArray *data(void)       { return at(XINDEX(CCSVField,data)).arr; }
private:
    void log(Time aTimeStamp);
};

//
// File format is:
// TimeStamp
// Name1,Name2,Name3,...
// Value1,Value2,Value3...
// Value1,Value2,Value3,...
// Value1,Value2,Value3,...
//
// Name is the CCSVField::name() for each field in the list.
// Value is the CCSVField::value() for each field in the list.
//
// Error codes:
//  0 - OK
//  1 - Could not open file.
//  2 - USB stick removed during logging.
//  3 - USB stick is full.
//
class CCSVWriter: public CExecutable {
public:
    XOBJECT_TYPE(CCSVWriter, 
		 "CSVWriter",
		 "CSV format writer",
		 (CCSVWriter_fields,
		  CCSVWriter_error,
		  CCSVWriter_store,
		  CCSVWriter_fileNamePattern,
		  CCSVWriter_fileName),
		 XFIELD(CCSVWriter, Q_PUBLIC, fields, 
			CArrayType::create(CCSVField::CCSVFieldType::singleton(), 0),
			""),
		 XFIELD(CCSVWriter,Q_PUBLIC,error,
			event_signed_type(),
			"Error code to set."),
		 XFIELD(CCSVWriter,Q_PUBLIC,store,
			event_bool_type(),
			"Active or not active."),
		 XFIELD(CCSVWriter,Q_PUBLIC,fileNamePattern,
			event_string_type(),
			"File name patter, %Y->Year %M ->Month %D->Day %h->hour %m->min %s->sec %d->seq_nr."),
		 XFIELD(CCSVWriter,Q_PUBLIC,fileName,
			event_string_type(),
			"Used file name")
	);
public:
    CCSVWriter(CExecutor* aExec, CBaseType *aType = CCSVWriterType::singleton());
    ~CCSVWriter(void);
    void execute(CExecutor* aExec);

private:
    CArray *fieldsArray(void) { return at(XINDEX(CCSVWriter,fields)).arr; }

    //
    // Store all data deined in 'fields'
    //
    void store(CExecutor* aExec);

    //
    // output file descriptor that we write to.
    //
    int mDescriptor;

    EventSigned mError;
    EventBool mStore;
    EventString mFileNamePattern;
    EventString mFileName;
};



#endif // __CSV__

