//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Magden LLC, California, USA, 2004, 2005, 2006, 2007.
//
#include "csv.hh"
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

XOBJECT_TYPE_BOOTSTRAP(CCSVWriter);

CCSVWriter::CCSVWriter(CExecutor* aExec, CBaseType *aType):
    CExecutable(aExec, aType),
    mDescriptor(-1),
    mError(this),
    mStore(this),
    mFileNamePattern(this),
    mFileName(this)
{ 
    CArrayType* t  = CArrayType::create(CCSVField::CCSVFieldType::singleton(), 0);
    CArray *a = new CArray(aExec, t, sizeof(CCSVField *), 0);

    mStore.putValue(aExec, false);
    mError.putValue(aExec, 0);
    mFileNamePattern.putValue(aExec, "hp.csv");
    mFileName.putValue(aExec, "");

    // Init fields to an empty array.
    put(aExec,XINDEX(CCSVWriter,fields), UArray(a));
    eventPut(aExec, XINDEX(CCSVWriter,error), &mError); 
    eventPut(aExec, XINDEX(CCSVWriter,store), &mStore); 
    eventPut(aExec, XINDEX(CCSVWriter,fileName), &mFileName); 
    eventPut(aExec, XINDEX(CCSVWriter,fileNamePattern), &mFileNamePattern); 
}


CCSVWriter::~CCSVWriter(void) 
{
    m1ReleaseArray(at(XINDEX(CCSVWriter,fields)).arr);
}


void CCSVWriter::store(CExecutor* aExec)
{
    char line[2048];
    char tmpline[256];
    struct tm *ct;
    time_t tmp;
    CArray* fields = at(XINDEX(CCSVWriter,fields)).arr;
    unsigned int arr_size = fields->size();
    string filename;
    string::size_type year_pos;
    string::size_type mon_pos;
    string::size_type day_pos;
    string::size_type hour_pos;
    string::size_type min_pos;
    string::size_type sec_pos;
    string::size_type seq_pos;
    char year[5];
    char mon[16];
    char day[3];
    char hour[3];
    char min[3];
    char sec[3];
    bool done;

    if (mDescriptor != -1)
	close(mDescriptor);

    //
    // Create a file name
    //
    tmp = time(0); 
    ct = localtime(&tmp);
    sprintf(year, "%.4d", ct->tm_year + 1900);
    sprintf(mon, "%.2d", ct->tm_mon + 1);
    sprintf(day, "%.2d", ct->tm_mday);
    sprintf(hour, "%.2d", ct->tm_hour);
    sprintf(min, "%.2d", ct->tm_min);
    sprintf(sec, "%.2d", ct->tm_sec);

    filename = mFileNamePattern.value();

    // %d -> 00000
    if ((seq_pos = filename.find("%d", 0))  != string::npos) {
	// Open up for five digits.
	filename.insert(seq_pos + 2, "   ");
    }

    if ((year_pos = filename.find("%Y", 0))  != string::npos) {
	filename.insert(year_pos + 2, "  ");
	filename.replace(year_pos, 4, year);
    }


    if ((mon_pos = filename.find("%M", 0)) != string::npos) filename.replace(mon_pos, 2, mon);
    if ((day_pos = filename.find("%D", 0)) != string::npos) filename.replace(day_pos, 2, day);
    if ((hour_pos = filename.find("%h", 0))!= string::npos) filename.replace(hour_pos, 2, hour);
    if ((min_pos = filename.find("%m", 0)) != string::npos) filename.replace(min_pos, 2, min);
    if ((sec_pos = filename.find("%s", 0)) != string::npos) filename.replace(sec_pos, 2, sec);


    //
    // If we have a %d. Iterate over any existing files until we find something
    // that is free.
    // 
    if (seq_pos != string::npos) {
	int i = 1;
	char seq_str[6];

	while(true) {
	    struct stat file_st;
	    sprintf(seq_str, "%.5d", i);
	    filename.replace(seq_pos, 5, seq_str);
	    if (stat(filename.c_str(), &file_st) == -1)
		break;

	    ++i;
	}
    }

    printf("Starting logging [%u] fields to file [%s].\n", arr_size, filename.c_str());

    mDescriptor = open(filename.c_str(), O_CREAT | O_WRONLY, 0666 );
    
    //
    // Check if something went wrong.
    //
    if (mDescriptor == -1) {
	DBGFMT("Cannot open file [%s] for writing: [%s]",
	       filename.c_str(),
	       strerror(errno));

	mError.putValue(aExec, 1);
	return;
    }
    
    DBGFMT("Setting filename to [%s]", filename.c_str());
    mFileName.putValue(aExec, filename);
    mError.putValue(aExec, 0);

    //
    // Write starting time.
    // 
    sprintf(line, "%.2d/%.2d/%.4d %.2d:%.2d:%.2d\r\n", 
	    ct->tm_mon + 1,
	    ct->tm_mday,
	    ct->tm_year + 1900,
	    ct->tm_hour,
	    ct->tm_min,
	    ct->tm_sec);
    write(mDescriptor, line, strlen(line));
    
    //
    // Write all field names
    //


    line[0] = 0;
    for (unsigned int i = 0; i < arr_size; ++i) {
      CCSVField *field = dynamic_cast<CCSVField *>(fieldsArray()->at(i).o);

	// Just add the channel if it is in use.
      //	printf("field[%s]\n",field->name());
	if (line[0] != 0) 
	  strcat(line, ",");

	strcat(line, field->name());
    }

    strcat(line, "\r\n");
    write(mDescriptor, line, strlen(line));

    //
    // Walk through all elements of all fields and dump them to file.
    //
    int field_ind = 0;
    do {
      line[0] = 0;
      done = true;

      //
      // Go through all fields and install their value.
      //
      for (unsigned int i = 0; i < arr_size; ++i) {
	CCSVField *field = dynamic_cast<CCSVField *>(fieldsArray()->at(i).o);

	if (line[0] != 0) 
	  strcat(line, ",");

	//
	// Only install value if we have not passed end of array for
	// this given field
	//
	if (field->data()->size() > field_ind) {
	  char tmp[32];
	  sprintf(tmp, field->format(), field->data()->at(field_ind).f);
	  strcat(line, tmp);
	  done = false;
	}
      }

      // Dump the line
      if (!done) {
	  //	printf("CSVWriter: Will log[%s]\n", line);
	strcat(line, "\r\n");
	write(mDescriptor, line, strlen(line));
      }
      field_ind++;
    } while(!done);

    fsync(mDescriptor);
    close(mDescriptor);
    mDescriptor = -1;
    //    printf("CSVWriter: Done\n");
    
    return;
}


void CCSVWriter::execute(CExecutor* aExec)
{
  if (mStore.updated() && mStore.value()) {
      mStore.putValue(aExec, false);
      store(aExec);
  }

  return;
}
