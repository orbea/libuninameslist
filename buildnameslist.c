/* buildnameslist.c - create files required for building libuninameslist
 * Copyright (C) 2003... George Williams <pfaedit@users.sourceforge.net>
 * Copyright (C) 2013... Joe Da Silva <digital@joescat.com>
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "buildnameslist.h"

#define UNUSED_PARAMETER(x) ((void)x)
#define BBUFFSIZE 2000

/* Build this program using: make buildnameslist		      */

/* buildnameslist reads from NamesList.txt and ListeDesNoms.txt which */
/* must be present in the current directory. Then, builds two arrays  */
/* of strings for each unicode character. One array contains the name */
/* of the character, the other holds annotations for each character.  */
/* Outputs nameslist.c containing these two sparse arrays:	      */
/* 2=={English=0, French=1} */
static char *uninames[2][17*65536];
static char *uniannot[2][17*65536];
/* There are approximately 25 names that changed (version 1 ->2), and */
/* a few more errors later. names2pt points to the name (after the %) */
/* and names2ln is the string length of the name if you only want the */
/* 2nd name without trailing annotations:			      */
static char names2pt[2][17*65536];
static char names2ln[2][17*65536];
static char names2cnt[2];

static struct block { long int start, end; char *name; struct block *next;}
	*head[2]={NULL,NULL}, *final[2]={NULL,NULL};

unsigned max_a, max_n;

static const char *lg[2] = { "", "FR" };
static const char *lgb[2] = { "UNICODE_EN_BLOCK_MAX", "UNICODE_FR_BLOCK_MAX" };
static const char *lgv[2] = { NL_VERSION, NFR_VERSION };


static int printcopyright2credits(FILE *out) {
    fprintf( out, "; Ces noms français sont utilisés pour confectionner\n");
    fprintf( out, ";\tles commentaires documentant chacun des caractères\n");
    fprintf( out, ";\tdont les poids de tri sont déterminés dans la table commune\n");
    fprintf( out, ";\tde la norme internationale ISO/CEI 14651. Cette dernière table\n");
    fprintf( out, ";\test normative. La présente liste est informative, jusqu’à ce que\n");
    fprintf( out, ";\tl’ISO/CEI 10646 ait été remise à niveau en français.\n;\n");
    fprintf( out, "; Contributions à la version %s française des noms de caractère :\n", NFR_VERSION);
    fprintf( out, ";\tJacques André, France\n");
    fprintf( out, ";\tPatrick Andries, Canada (Québec)\n");
    fprintf( out, ";\tBernard Chauvois, France\n");
    fprintf( out, ";\tKarljürgen Feuerherm, Canada (Ontario)\n");
    fprintf( out, ";\tAlain LaBonté, Canada (Québec)\n");
    fprintf( out, ";\tMarc Lodewijck, Belgique\n");
    fprintf( out, ";\tMichel Suignard, États-Unis d’Amérique\n");
    fprintf( out, ";\tFrançois Yergeau, Canada (Québec)\n");
    return( 1 );
}

static int printcopyright2(FILE *out) {
    fprintf( out, "\n/*\n");
    fprintf( out, "; Standard Unicode %s ou\n", NFR_VERSION);
    fprintf( out, ";	Norme internationale ISO/CEI 10646\n;\n");
    printcopyright2credits(out);
    fprintf( out, "*/\n\n");
    return( 1 );
}

static int printcopyright1(FILE *out, int is_fr) {
    /* Copyright notice for unicode NamesList.txt - 2021 */
    fprintf( out, "\n/*\n");
    fprintf( out, "The data contained in these arrays were derived from data contained in\n");
    fprintf( out, "NamesList.txt which came from www.unicode.org. Below is the copyright\n");
    fprintf( out, "notice for the information given:\n\n");
    fprintf( out, "Copyright © 1991-2021 Unicode®, Inc. All rights reserved.\n");
    fprintf( out, "Distributed under the Terms of Use in https://www.unicode.org/copyright.html.\n\n");
    fprintf( out, "Permission is hereby granted, free of charge, to any person obtaining\n");
    fprintf( out, "a copy of the Unicode data files and any associated documentation\n");
    fprintf( out, "(the \"Data Files\") or Unicode software and any associated documentation\n");
    fprintf( out, "(the \"Software\") to deal in the Data Files or Software\n");
    fprintf( out, "without restriction, including without limitation the rights to use,\n");
    fprintf( out, "copy, modify, merge, publish, distribute, and/or sell copies of\n");
    fprintf( out, "the Data Files or Software, and to permit persons to whom the Data Files\n");
    fprintf( out, "or Software are furnished to do so, provided that either\n");
    fprintf( out, "(a) this copyright and permission notice appear with all copies\n");
    fprintf( out, "of the Data Files or Software, or\n");
    fprintf( out, "(b) this copyright and permission notice appear in associated\n");
    fprintf( out, "Documentation.\n\n");
    fprintf( out, "THE DATA FILES AND SOFTWARE ARE PROVIDED \"AS IS\", WITHOUT WARRANTY OF\n");
    fprintf( out, "ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\n");
    fprintf( out, "WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n");
    fprintf( out, "NONINFRINGEMENT OF THIRD PARTY RIGHTS.\n");
    fprintf( out, "IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS\n");
    fprintf( out, "NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL\n");
    fprintf( out, "DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\n");
    fprintf( out, "DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\n");
    fprintf( out, "TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\n");
    fprintf( out, "PERFORMANCE OF THE DATA FILES OR SOFTWARE.\n\n");
    fprintf( out, "Except as contained in this notice, the name of a copyright holder\n");
    fprintf( out, "shall not be used in advertising or otherwise to promote the sale,\n");
    fprintf( out, "use or other dealings in these Data Files or Software without prior\n");
    fprintf( out, "written authorization of the copyright holder.\n\n");
    if ( is_fr<0 || is_fr==1 ) {
	fprintf( out, "\n");
	printcopyright2credits(out);
    }
    fprintf( out, "*/\n\n");
    return( 1 );
}

static char *myfgets(char *buf,int bsize,FILE *file) {
    /* NamesList.txt uses CR as a line separator */
    int ch;
    char *pt, *end = buf+bsize-2;

    for ( pt=buf; pt<end && (ch=getc(file))!=EOF && ch!='\n' && ch!='\r'; )
	*pt++ = (char)(ch);
    if ( ch=='\n' || ch=='\r' ) {
	*pt++='\n';
	if ( ch=='\r' ) {
	    ch=getc(file);
	    if ( ch!='\n' )
		ungetc(ch,file);
	}
    }
    if ( pt==buf && ch==EOF )
return( NULL );
    *pt = '\0';
return( buf );
}

static void InitArrays(void) {
    int i,j;
    for (i=0; i<2; i++) for (j=0; j<17*65536; j++) {
	uninames[i][j] = uniannot[i][j] = NULL;
	names2pt[i][j] = names2ln[i][j] = (char)(127);
    }
    names2cnt[0] = names2cnt[1] = 0;
}

static void FreeArrays(void) {
    int i,j;
    for (i=0; i<2; i++) for (j=0; j<17*65536; j++) {
	free(uninames[i][j]); free(uniannot[i][j]);
    }

    for (i=0; i<2; i++) {
	struct block *temp;
	while ( head[i]!=NULL ) {
	    if ( head[i]->name!=NULL ) free(head[i]->name);
	    temp=head[i]->next; free(head[i]); head[i]=temp;
	}
    }
}

static int ReadNamesList(void) {
    char buffer[BBUFFSIZE];
    FILE *nl;
    long int a_char = -1, first, last;
    char *end, *namestart, *pt, *temp;
    struct block *cur;
    int i, j;
    static char *nameslistfiles[] = { "NamesList.txt", "ListeDesNoms.txt", NULL };
    static char *nameslistlocs[] = {
	"http://www.unicode.org/Public/UNIDATA/NamesList.txt",
	"http://hapax.qc.ca/ListeNoms-14.0.0.txt (latin base char set)"
    };

    buffer[sizeof(buffer)-1]=0;
    for ( i=0; nameslistfiles[i]!=NULL; ++i ) {
	nl = fopen( nameslistfiles[i],"r" );
	if ( nl==NULL ) {
	    fprintf( stderr, "Cannot find %s. Please copy it from\n\t%s\n", nameslistfiles[i], nameslistlocs[i] );
	    goto errorReadNamesListFO;
	}
	while ( myfgets(buffer,BBUFFSIZE-1,nl)!=NULL ) {
	    if ( buffer[0]=='@' ) {
		if ( buffer[1]=='+' && buffer[2]=='\t' ) {
		    /* This is a Notice_line, @+ */
		    if ( a_char>=0 && a_char<(int)(sizeof(uniannot[0])/sizeof(uniannot[0][0])) ) {
			for ( pt=buffer; *pt && *pt!='\r' && *pt!='\n' ; ++pt );
			if ( *pt=='\r' ) *pt='\n';
			if ( uniannot[i][a_char]==NULL )
			    uniannot[i][a_char] = strdup(buffer+2);
			else {
			    temp = (char *)(realloc(uniannot[i][a_char],strlen(uniannot[i][a_char])+strlen(buffer+2)+1));
			    if ( temp==NULL ) goto errorReadNamesList;
			    strcat(temp,buffer+2);
			    uniannot[i][a_char] = temp;
			}
	continue;
		    } else {
		    ;
		    }
		}
		a_char = -1;
		if ( buffer[1]=='@' && buffer[2]=='\t' ) {
		    /* This is a Block_Header {first...last}, @@ */
		    first = strtol(buffer+3,&end,16);
		    if ( *end=='\t' ) {
			namestart = end+1;
			for ( pt=namestart; *pt!='\0' && *pt!='\t' ; ++pt );
			if ( *pt=='\t' ) {
			    *pt = '\0';
			    last = strtol(pt+1,&end,16);
			    if ( last>first ) {
				/* found a block, record info */
				cur = (struct block *)(malloc(sizeof(struct block)));
				if ( cur==NULL ) goto errorReadNamesList;
				cur->start = first;
				cur->end = last;
				cur->name = strdup(namestart);
				if ( final[i]==NULL )
				    head[i] = cur;
				else
				    final[i]->next = cur;
				final[i] = cur;
			    }
			}
		    }
		}
	continue;
	    } else if ( buffer[0]==';' ) {
		/* comment, ignore */
	continue;
	    } else if ( (buffer[0]>='0' && buffer[0]<='9') || (buffer[0]>='A' && buffer[0]<='F') ) {
		a_char = strtol(buffer,&end,16);
		if ( *end!='\t' )
	continue;
		else if ( end[1]=='<' )
	continue;
		namestart = end+1;
		for ( pt=namestart; *pt && *pt!='\r' && *pt!='\n' && *pt!='\t' && *pt!=';' ; ++pt );
		*pt = '\0';
		if ( a_char>=0 && a_char<(int)(sizeof(uninames[0])/sizeof(uninames[0][0])) )
		    uninames[i][a_char] = strdup(namestart);
	    } else if ( a_char==-1 ) {
	continue;
	    } else if ( buffer[0]=='\t' && buffer[1]==';' ) {
	continue;		/* comment */
	    } else if ( a_char>=0 && a_char<(int)(sizeof(uniannot[0])/sizeof(uniannot[0][0])) ) {
		for ( pt=buffer; *pt && *pt!='\r' && *pt!='\n' ; ++pt );
		if ( *pt=='\r' ) *pt='\n';
		if ( uniannot[i][a_char]==NULL )
		    uniannot[i][a_char] = strdup(buffer);
		else {
		    temp = (char *)(realloc(uniannot[i][a_char],strlen(uniannot[i][a_char])+strlen(buffer)+1));
		    if ( temp==NULL ) goto errorReadNamesList;
		    strcat(temp,buffer);
		    uniannot[i][a_char] = temp;
		}
	    }
	}
	fclose(nl);

	/* search for possible normalized aliases. Assume 1st annotation line */
	for ( a_char=0; a_char<17*65536; ++a_char ) if ( uniannot[i][a_char]!=NULL ) {
	    pt = uniannot[i][a_char];
	    if ( *pt=='\t' && *++pt=='%' && *++pt==' ' ) {
		for ( j=-1; *pt!='\n' && *pt!='\0'; ++j,++pt );
		if ( j>0 && j<127 ) {
		    names2pt[i][a_char] = 3;
		    names2ln[i][a_char] = (char)(j);
		    names2cnt[i]++;
		}
	    }
	}
    }
    return( 1 );

errorReadNamesList:
    fprintf( stderr,"Out of memory\n" );
    fclose(nl);
errorReadNamesListFO:
    return( 0 );
}

static void dumpstring(char *str,FILE *out) {
    do {
	putc( '"', out);
	for ( ; *str!='\n' && *str!='\0'; ++str ) {
	    if ( *str=='"' || *str=='\\' )
		putc('\\',out);
	    putc(*str,out);
	}
	if ( *str=='\n' && str[1]!='\0' )
	    fprintf( out, "\\n\"\n\t" );
	else
	    putc('"',out);
	if ( *str=='\n' ) ++str;
    } while ( *str );
}

static int dumpinit(FILE *out, FILE *header, int is_fr) {
    /* is_fr => 0=english, 1=french */
    int i, l;
    long a_char;

    l = is_fr; if ( is_fr<0 ) l = 0;

    fprintf( out, "#include <stdio.h>\n" );
    if ( is_fr<1 )
	fprintf( out, "#include \"uninameslist.h\"\n" );
    else
	fprintf( out, "#include \"uninameslist-fr.h\"\n" );
    /* note dll follows uninameslist*.h file */
    fprintf( out, "#include \"nameslist-dll.h\"\n\n" );

    fprintf( out, "/* This file was generated using the program 'buildnameslist.c' */\n\n" );

    if ( is_fr<1 ) printcopyright1(out, is_fr);
    if ( is_fr==1 ) printcopyright2(out);

    /* Added functions available in libuninameslist version 0.3 and higher. */
    fprintf( out, "/* Retrieve a pointer to the name of a Unicode codepoint. */\n" );
    fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_name%s(unsigned long uni) {\n", lg[l] );
    fprintf( out, "\tconst char *pt=NULL;\n\n" );
    fprintf( out, "\tif ( uni<0x110000 )\n" );
    fprintf( out, "\t\tpt=UnicodeNameAnnot%s[uni>>16][(uni>>8)&0xff][uni&0xff].name;\n", lg[l] );
    fprintf( out, "\treturn( pt );\n}\n\n" );
    fprintf( out, "/* Retrieve a pointer to annotation details of a Unicode codepoint. */\n" );
    fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_annot%s(unsigned long uni) {\n", lg[l] );
    fprintf( out, "\tconst char *pt=NULL;\n\n" );
    fprintf( out, "\tif ( uni<0x110000 )\n" );
    fprintf( out, "\t\tpt=UnicodeNameAnnot%s[uni>>16][(uni>>8)&0xff][uni&0xff].annot;\n", lg[l] );
    fprintf( out, "\treturn( pt );\n}\n\n" );
    fprintf( out, "/* Retrieve Nameslist.txt version number. */\n" );
    fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_NamesListVersion%s(void) {\n",lg[l] );
    fprintf( out, "\treturn( \"Nameslist-Version: %s\" );\n}\n\n", lgv[l] );
    /* Added functions available in libuninameslist version 0.4 and higher. */
    fprintf( out, "\n/* These functions are available in libuninameslist-0.4.20140731 and higher */\n\n" );
    fprintf( out, "/* Return number of blocks in this NamesList. */\n" );
    fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_blockCount%s(void) {\n", lg[l] );
    fprintf( out, "\treturn( %s );\n}\n\n", lgb[l] );
    fprintf( out, "/* Return block number for this unicode value, -1 if unlisted unicode value */\n" );
    fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_blockNumber%s(unsigned long uni) {\n", lg[l] );
    fprintf( out, "\tint i;\n\n\tif ( uni<0x110000 ) {\n" );
    fprintf( out, "\t\tfor (i=0; i<%s; i++) {\n", lgb[l] );
    fprintf( out, "\t\t\tif ( uni<(unsigned long)(UnicodeBlock%s[i].start) ) break;\n", lg[l] );
    fprintf( out, "\t\t\tif ( uni<=(unsigned long)(UnicodeBlock%s[i].end) ) return( i );\n", lg[l] );
    fprintf( out, "\t\t}\n\t}\n\treturn( -1 );\n}\n\n" );
    fprintf( out, "/* Return unicode value starting this Unicode block (-1 if bad uniBlock). */\n" );
    fprintf( out, "UN_DLL_EXPORT\nlong uniNamesList_blockStart%s(int uniBlock) {\n", lg[l] );
    fprintf( out, "\tif ( uniBlock<0 || uniBlock>=%s )\n\t\treturn( -1 );\n", lgb[l] );
    fprintf( out, "\treturn( (long)(UnicodeBlock%s[uniBlock].start) );\n}\n\n", lg[l] );
    fprintf( out, "/* Return unicode value ending this Unicode block (-1 if bad uniBlock). */\n" );
    fprintf( out, "UN_DLL_EXPORT\nlong uniNamesList_blockEnd%s(int uniBlock) {\n", lg[l] );
    fprintf( out, "\tif ( uniBlock<0 || uniBlock>=%s )\n\t\treturn( -1 );\n", lgb[l] );
    fprintf( out, "\treturn( (long)(UnicodeBlock%s[uniBlock].end) );\n}\n\n", lg[l] );
    fprintf( out, "/* Return a pointer to the blockname for this unicode block. */\n" );
    fprintf( out, "UN_DLL_EXPORT\nconst char * uniNamesList_blockName%s(int uniBlock) {\n", lg[l] );
    fprintf( out, "\tif ( uniBlock<0 || uniBlock>=%s )\n\t\treturn( NULL );\n", lgb[l] );
    fprintf( out, "\treturn( UnicodeBlock%s[uniBlock].name );\n}\n\n", lg[l] );

    fprintf( out, "\n/* These functions are available in libuninameslist-20171118 and higher */\n\n" );
    fprintf( out, "/* Return count of how many names2 are found in this version of library */\n" );
    fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_names2cnt%s(void) {\n", lg[l] );
    fprintf( out, "\treturn( %d );\n}\n\n", names2cnt[l] );

    if ( names2cnt[l]>0 ) {
	fprintf( out, "static const unsigned long unicode_name2code%s[] = {", lg[l] );
	for ( i=0,a_char=0; i<names2cnt[l] && a_char<0x110000; ++a_char ) {
	    if ( names2pt[l][a_char]>=0 && names2pt[l][a_char]<127) {
		if ( i&7 ) fprintf( out, " " ); else fprintf( out, "\n\t" );
		if ( a_char<=0xffff )
		    fprintf( out, "0x%04X", (int)(a_char) );
		else
		    fprintf( out, "%ld", a_char );
		if ( ++i!=names2cnt[l] ) fputc( ',', out );
	    }
	}
	fprintf( out, "\n};\n\n" );

	fprintf( out, "static const char unicode_name2vals%s[] = {", lg[l] );
	for ( i=0,a_char=0; i<names2cnt[l] && a_char<0x110000; ++a_char ) {
	    if ( names2pt[l][a_char]>=0 && names2pt[l][a_char]<127) {
		if ( i&7 ) fprintf( out, " " ); else fprintf( out, "\n\t" );
		fprintf( out, "%d,%d%s", names2pt[l][a_char], names2ln[l][a_char], ++i!=names2cnt[l]?",":"" );
	    }
	}
	fprintf( out, "\n};\n\n" );
    }

    fprintf( out, "/* Return unicode value with names2 (0<=count<uniNamesList_names2cnt(). */\n" );
    fprintf( out, "UN_DLL_EXPORT\nlong uniNamesList_names2val%s(int count) {\n", lg[l] );
    if ( names2cnt[l]<=0 )
	fprintf( out, "\treturn( -1 );\n}\n\n" );
    else {
	fprintf( out, "\tif ( count<0 || count>=%d ) return( -1 );\n", names2cnt[l] );
	fprintf( out, "\treturn( (long)(unicode_name2code%s[count]) );\n}\n\n", lg[l] );
    }
    fprintf( out, "/* Return list location for this unicode value. Return -1 if not found. */\n" );
    fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_names2getU%s(unsigned long uni) {\n", lg[l] );
    if ( names2cnt[l]>0 ) {
	fprintf( out, "\tint i;\n\n\tif ( uni<0x110000 ) for ( i=0; i<%d; ++i ) {\n", names2cnt[l] );
	fprintf( out, "\t\tif ( uni==unicode_name2code%s[i] ) return( i );\n", lg[l] );
	fprintf( out, "\t\tif ( uni<unicode_name2code%s[i] ) break;\n\t}\n", lg[l] );
    }
    fprintf( out, "\treturn( -1 );\n}\n\n" );
    fprintf( out, "/* Stringlength of names2. Use this if you want to truncate annotations */\n" );
    fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_names2lnC%s(int count) {\n", lg[l] );
    if ( names2cnt[l]<=0 )
	fprintf( out, "\treturn( -1 );\n}\n\n" );
    else {
	fprintf( out, "\tif ( count<0 || count>=%d ) return( -1 );\n", names2cnt[l] );
	fprintf( out, "\treturn( (int)(unicode_name2vals%s[(count<<1)+1]) );\n}\n\n", lg[l] );
    }
    fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_names2lnU%s(unsigned long uni) {\n", lg[l] );
    if ( names2cnt[l]<=0 )
	fprintf( out, "\treturn( -1 );\n}\n\n" );
    else
	fprintf( out, "\treturn( uniNamesList_names2lnC%s(uniNamesList_names2getU%s(uni)) );\n}\n\n", lg[l], lg[l] );
    fprintf( out, "/* Return pointer to start of normalized alias names2 within annotation */\n" );
    fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_names2anC%s(int count) {\n", lg[l] );
    if ( names2cnt[l]<=0 )
	fprintf( out, "\treturn( NULL );\n}\n\n" );
    else {
	fprintf( out, "\tint c;\n\tconst char *pt;\n\n" );
	fprintf( out, "\tif ( count<0 || count>=%d ) return( NULL );\n", names2cnt[l] );
	fprintf( out, "\tc = unicode_name2vals%s[count<<1];\n", lg[l] );
	fprintf( out, "\tpt = uniNamesList_annot((unsigned long)(uniNamesList_names2val%s(count)));\n", lg[l] );
	fprintf( out, "\twhile ( --c>=0 ) ++pt;\n\treturn( pt );\n}\n\n" );
    }
    fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_names2anU%s(unsigned long uni) {\n", lg[l] );
    if ( names2cnt[l]<=0 )
	fprintf( out, "\treturn( NULL );\n}\n\n" );
    else
	fprintf( out, "\treturn( uniNamesList_names2anC%s(uniNamesList_names2getU%s(uni)) );\n}\n\n\n", lg[l], lg[l] );

    if ( is_fr==0 ) {
	fprintf( out, "/* These functions are available in libuninameslist-20200413 and higher */\n\n" );
	fprintf( out, "UN_DLL_LOCAL\nint uniNamesList_haveFR(unsigned int lang) {\n" );
	fprintf( out, "#ifdef WANTLIBOFR\n\tif ( lang==1 ) return( 1 );\n#endif\n\treturn( 0 );\n}\n\n" );
	fprintf( out, "#ifndef WANTLIBOFR\n/* make these internal stubs since there's no French lib */\n" );
	fprintf( out, "UN_DLL_LOCAL const char *uniNamesList_NamesListVersionFR(void) {return( NULL );}\n" );
	fprintf( out, "UN_DLL_LOCAL const char *uniNamesList_nameFR(unsigned long uni) {return( NULL );}\n" );
	fprintf( out, "UN_DLL_LOCAL const char *uniNamesList_annotFR(unsigned long uni) {return( NULL );}\n" );
	fprintf( out, "UN_DLL_LOCAL int uniNamesList_blockCountFR(void) {return( -1 );}\n" );
	fprintf( out, "UN_DLL_LOCAL int uniNamesList_blockNumberFR(unsigned long uni) {return( -1 );}\n" );
	fprintf( out, "UN_DLL_LOCAL long uniNamesList_blockStartFR(int uniBlock) {return( -1 );}\n" );
	fprintf( out, "UN_DLL_LOCAL long uniNamesList_blockEndFR(int uniBlock) {return( -1 );}\n" );
	fprintf( out, "UN_DLL_LOCAL const char *uniNamesList_blockNameFR(int uniBlock) {return( NULL );}\n" );
	fprintf( out, "#endif\n\n/* Return language codes available from libraries. 0=English, 1=French. */\n" );
	fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_Languages(unsigned int lang) {\n" );
	fprintf( out, "\tif ( uniNamesList_haveFR(lang) )\n\t\treturn( \"FR\" );\n" );
	fprintf( out, "\telse if ( lang==0 )\n\t\treturn( \"EN\" );\n\treturn( NULL );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_NamesListVersionAlt(unsigned int lang) {\n" );
	fprintf( out, "\tif ( uniNamesList_haveFR(lang) )\n\t\treturn( (const char *)(uniNamesList_NamesListVersionFR()) );\n" );
	fprintf( out, "\telse if ( lang==0 )\n\t\treturn( uniNamesList_NamesListVersion() );\n\treturn( NULL );\n}\n\n" );
	fprintf( out, "/* Return pointer to name/annotation for this unicode value using lang. */\n" );
	fprintf( out, "/* Return English if language does not have information for this Ucode. */\n" );
	fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_nameAlt(unsigned long uni, unsigned int lang) {\n" );
	fprintf( out, "\tconst char *pt=NULL;\n\n\tif ( uni<0x110000 ) {\n" );
	fprintf( out, "\t\tif ( uniNamesList_haveFR(lang) )\n\t\t\tpt=(const char *)(uniNamesList_nameFR(uni));\n" );
	fprintf( out, "\t\tif ( pt==NULL )\n\t\t\tpt=uniNamesList_name(uni);\n\t}\n\treturn( pt );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_annotAlt(unsigned long uni, unsigned int lang) {\n" );
	fprintf( out, "\tconst char *pt=NULL;\n\n\tif ( uni<0x110000 ) {\n" );
	fprintf( out, "\t\tif ( uniNamesList_haveFR(lang) )\n\t\t\tpt=(const char *)(uniNamesList_annotFR(uni));\n" );
	fprintf( out, "\t\tif ( pt==NULL )\n\t\t\tpt=uniNamesList_annot(uni);\n\t}\n\treturn( pt );\n}\n\n" );
	fprintf( out, "/* Returns 2 lang pointers to names/annotations for this unicode value, */\n" );
	fprintf( out, "/* Return str0=English, and str1=language_version (or NULL if no info). */\n" );
	fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_nameBoth(unsigned long uni, unsigned int lang, const char **str0, const char **str1) {\n" );
	fprintf( out, "\tint error=-1;\n\t*str0=*str1=NULL;\n\n\tif ( uni<0x110000 ) {\n" );
	fprintf( out, "\t\terror=0;\n\t\t*str0=uniNamesList_name(uni);\n" );
	fprintf( out, "\t\tif ( uniNamesList_haveFR(lang) )\n\t\t\t*str1=(const char *)(uniNamesList_nameFR(uni));\n" );
	fprintf( out, "\t\telse if ( lang==0 )\n\t\t\t*str1=*str0;\n\t}\n\treturn( error );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_annotBoth(unsigned long uni, unsigned int lang, const char **str0, const char **str1) {\n" );
	fprintf( out, "\tint error=-1;\n\t*str0=*str1=NULL;\n\n" );
	fprintf( out, "\tif ( uni<0x110000 ) {\n\t\terror=0;\n\t\t*str0=uniNamesList_annot(uni);\n" );
	fprintf( out, "\t\tif ( uniNamesList_haveFR(lang) )\n\t\t\t*str1=(const char *)(uniNamesList_annotFR(uni));\n" );
	fprintf( out, "\t\telse if ( lang==0 )\n\t\t\t*str1=*str0;\n\t}\n\treturn( error );\n}\n\n" );
	fprintf( out, "/* Common access. Blocklists won't sync if they are different versions. */\n" );
	fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_blockCountAlt(unsigned int lang) {\n" );
	fprintf( out, "\tint c=-1;\n\n\tif ( uniNamesList_haveFR(lang) )\n\t\tc=(int)(uniNamesList_blockCountFR());\n" );
	fprintf( out, "\tif ( c<0 )\n\t\tc=UNICODE_EN_BLOCK_MAX;\n\treturn( c );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nlong uniNamesList_blockStartAlt(int uniBlock, unsigned int lang) {\n" );
	fprintf( out, "\tlong c=-1;\n\n\tif ( uniNamesList_haveFR(lang) )\n\t\tc=(long)(uniNamesList_blockStartFR(uniBlock));\n" );
	fprintf( out, "\tif ( c<0 )\n\t\tc=uniNamesList_blockStart(uniBlock);\n\treturn( c );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nlong uniNamesList_blockEndAlt(int uniBlock, unsigned int lang) {\n" );
	fprintf( out, "\tlong c=-1;\n\n\tif ( uniNamesList_haveFR(lang) )\n" );
	fprintf( out, "\t\tc=(long)(uniNamesList_blockEndFR(uniBlock));\n\tif ( c<0 )\n" );
	fprintf( out, "\t\tc=uniNamesList_blockEnd(uniBlock);\n\treturn( c );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nconst char *uniNamesList_blockNameAlt(int uniBlock, unsigned int lang) {\n" );
	fprintf( out, "\tconst char *pt=NULL;\n\n\tif ( uniNamesList_haveFR(lang) )\n" );
	fprintf( out, "\t\tpt=(const char *)(uniNamesList_blockNameFR(uniBlock));\n\tif ( pt==NULL )\n" );
	fprintf( out, "\t\tpt=uniNamesList_blockName(uniBlock);\n\treturn( pt );\n}\n\n" );
	fprintf( out, "UN_DLL_EXPORT\nint uniNamesList_blockNumberBoth(unsigned long uni, unsigned int lang, int *bn0, int *bn1) {\n" );
	fprintf( out, "\tint error=-1;\n\n\t*bn0=*bn1=-1;\n\tif ( uni<0x110000 ) {\n" );
	fprintf( out, "\t\terror=0;\n\t\t*bn0=uniNamesList_blockNumber(uni);\n" );
	fprintf( out, "\t\tif ( uniNamesList_haveFR(lang) )\n\t\t\t*bn1=(int)(uniNamesList_blockNumberFR(uni));\n" );
	fprintf( out, "\t\telse if ( lang==0 )\n\t\t\t*bn1=*bn0;\n\t}\n\treturn( error );\n}\n\n\n" );
    }

    fprintf( out, "UN_DLL_LOCAL\nstatic const struct unicode_nameannot nullarray%s[] = {\n", lg[l] );
    for ( i=0; i<256/4 ; ++i )
	fprintf( out, "\t{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },\n" );
    fprintf( out, "\t{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }\n" );
    fprintf( out, "};\n\n" );
    fprintf( out, "UN_DLL_LOCAL\nstatic const struct unicode_nameannot nullarray2%s[] = {\n", lg[l] );
    for ( i=0; i<256/4 ; ++i )
	fprintf( out, "\t{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL },\n" );
    fprintf( out, "\t{ NULL, NULL }, { NULL, NULL },\n" );
    if ( is_fr ) {
	fprintf( out, "\t{ NULL, \"\t* on est assuré que la valeur ?FFFE n'est en aucune façon un caractère Unicode\" },\n" );
	fprintf( out, "\t{ NULL, \"\t* on est assuré que la valeur ?FFFF n'est en aucune façon un caractère Unicode\" },\n" );
    } else {
	fprintf( out, "\t{ NULL, \"\t* the value ?FFFE is guaranteed not to be a Unicode character at all\" },\n" );
	fprintf( out, "\t{ NULL, \"\t* the value ?FFFF is guaranteed not to be a Unicode character at all\" },\n" );
    }
    fprintf( out, "};\n\n" );
    fprintf( out, "UN_DLL_LOCAL\nstatic const struct unicode_nameannot * const nullnullarray%s[] = {\n", lg[l] );
    for ( i=0; i<256/8 ; ++i )
	fprintf( out, "\tnullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s,\n", \
			lg[l], lg[l], lg[l], lg[l], lg[l], lg[l], lg[l], lg[l] );
    fprintf( out, "\tnullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray%s, nullarray2%s\n",
			lg[l], lg[l], lg[l], lg[l], lg[l], lg[l], lg[l], lg[l] );
    fprintf( out, "};\n\n" );

    if ( is_fr==1 ) {
	/* default Nameslist.txt language=EN file holds these additional functions */
	fprintf( header, "#ifndef UN_NAMESLIST_FR_H\n" );
	fprintf( header, "# define UN_NAMESLIST_FR_H\n\n" );
    } else {
	fprintf( header, "#ifndef UN_NAMESLIST_H\n" );
	fprintf( header, "# define UN_NAMESLIST_H\n\n" );
    }
    fprintf( header, "/* This file was generated using the program 'buildnameslist.c' */\n\n" );
    fprintf( header, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n" );
    if ( is_fr!=0 ) fprintf( header, "#ifndef UN_NAMESLIST_H\n" );
    fprintf( header, "struct unicode_block {\n\tint start, end;\n\tconst char *name;\n};\n\n" );
    fprintf( header, "struct unicode_nameannot {\n\tconst char *name, *annot;\n};\n" );
    if ( is_fr!=0 ) fprintf( header, "#endif\n" );
    fprintf( header, "\n" );
    return( 1 );
}

static int dumpend(FILE *header, int is_fr) {
    int l;

    l = is_fr; if ( is_fr<0 ) l = 0;

    fprintf( header, "\n/* Index by: UnicodeNameAnnot%s[(uni>>16)&0x1f][(uni>>8)&0xff][uni&0xff] */\n", lg[l] );
    fprintf( header, "\n/* At the beginning of lines (after a tab) within the annotation string, a: */\n" );
    fprintf( header, "/*  * should be replaced by a bullet U+2022 */\n" );
    fprintf( header, "/*  %% should be replaced by a reference mark U+203B */\n" );
    fprintf( header, "/*  x should be replaced by a right arrow U+2192 */\n" );
    fprintf( header, "/*  ~ should be replaced by a swung dash U+2053 */\n" );
    fprintf( header, "/*  : should be replaced by an equivalent U+2261 */\n" );
    fprintf( header, "/*  # should be replaced by an approximate U+2248 */\n" );
    fprintf( header, "/*  = should remain itself */\n\n" );

    /* default Nameslist.txt language=EN file holds these additional functions */
    /* Added functions available in libuninameslist version 0.3 and higher. */
    /* Maintain this sequence for old-programs-binary-backwards-compatibility. */
    fprintf( header, "/* Return a pointer to the name for this unicode value */\n" );
    fprintf( header, "/* This value points to a constant string inside the library */\n" );
    fprintf( header, "const char *uniNamesList_name%s(unsigned long uni);\n\n", lg[l] );
    fprintf( header, "/* Returns pointer to the annotations for this unicode value */\n" );
    fprintf( header, "/* This value points to a constant string inside the library */\n" );
    fprintf( header, "const char *uniNamesList_annot%s(unsigned long uni);\n\n", lg[l] );
    fprintf( header, "/* Return a pointer to the Nameslist.txt version number. */\n" );
    fprintf( header, "/* This value points to a constant string inside the library */\n" );
    fprintf( header, "const char *uniNamesList_NamesListVersion%s(void);\n\n", lg[l] );
    /* Added functions available in libuninameslist version 0.4 and higher. */
    fprintf( header, "\n/* These functions are available in libuninameslist-0.4.20140731 and higher */\n\n" );
    fprintf( header, "/* Version information for this <uninameslist.h> include file */\n" );
    if ( is_fr==0 ) {
	fprintf( header, "#define LIBUNINAMESLIST_MAJOR\t%d\n", LU_VERSION_MJ );
	fprintf( header, "#define LIBUNINAMESLIST_MINOR\t%d\n\n", LU_VERSION_MN );
	fprintf( header, "/* Return number of blocks in this NamesList (Version %s). */\n", NL_VERSION );
    } else if ( is_fr==1 ) {
	fprintf( header, "#define LIBUNINAMESLIST_FR_MAJOR\t%d\n", LFR_VERSION_MJ );
	fprintf( header, "#define LIBUNINAMESLIST_FR_MINOR\t%d\n\n", LFR_VERSION_MN );
	fprintf( header, "/* Return number of blocks in this NamesList (Version %s). */\n", NFR_VERSION );
    }
    fprintf( header, "int uniNamesList_blockCount%s(void);\n\n", lg[l] );
    fprintf( header, "/* Return block number for this unicode value (-1 if bad unicode value) */\n" );
    fprintf( header, "int uniNamesList_blockNumber%s(unsigned long uni);\n\n", lg[l] );
    fprintf( header, "/* Return unicode value starting this Unicode block (bad uniBlock = -1) */\n" );
    fprintf( header, "long uniNamesList_blockStart%s(int uniBlock);\n\n", lg[l] );
    fprintf( header, "/* Return unicode value ending this Unicode block (-1 if bad uniBlock). */\n" );
    fprintf( header, "long uniNamesList_blockEnd%s(int uniBlock);\n\n", lg[l] );
    fprintf( header, "/* Return a pointer to the blockname for this unicode block. */\n" );
    fprintf( header, "/* This value points to a constant string inside the library */\n" );
    fprintf( header, "const char * uniNamesList_blockName%s(int uniBlock);\n", lg[l] );
    if ( is_fr!=0 ) fprintf( header, "\n#define UnicodeNameAnnot UnicodeNameAnnot%s\n", lg[l] );

    fprintf( header, "\n/* These functions are available in libuninameslist-20180408 and higher */\n\n" );
    fprintf( header, "/* Return count of how many names2 are found in this version of library */\n" );
    fprintf( header, "int uniNamesList_names2cnt%s(void);\n\n", lg[l] );
    fprintf( header, "/* Return list location for this unicode value. Return -1 if not found. */\n" );
    fprintf( header, "int uniNamesList_names2getU%s(unsigned long uni);\n\n", lg[l] );
    fprintf( header, "/* Return unicode value with names2 (0<=count<uniNamesList_names2cnt(). */\n" );
    fprintf( header, "long uniNamesList_names2val%s(int count);\n\n", lg[l] );
    fprintf( header, "/* Stringlength of names2. Use this if you want to truncate annotations */\n" );
    fprintf( header, "int uniNamesList_names2lnC%s(int count);\n", lg[l] );
    fprintf( header, "int uniNamesList_names2lnU%s(unsigned long uni);\n\n", lg[l] );
    fprintf( header, "/* Return pointer to start of normalized alias names2 within annotation */\n" );
    fprintf( header, "const char *uniNamesList_names2anC%s(int count);\n", lg[l] );
    fprintf( header, "const char *uniNamesList_names2anU%s(unsigned long uni);\n\n", lg[l] );

    if ( is_fr==0 ) {
	fprintf( header, "/* These functions are available in libuninameslist-20200413 and higher */\n\n" );
	fprintf( header, "/* Return language codes available from libraries. 0=English, 1=French. */\n" );
	fprintf( header, "const char *uniNamesList_Languages(unsigned int lang);\n" );
	fprintf( header, "const char *uniNamesList_NamesListVersionAlt(unsigned int lang);\n\n" );
	fprintf( header, "/* Return pointer to name/annotation for this unicode value using lang. */\n" );
	fprintf( header, "/* Return English if language does not have information for this Ucode. */\n" );
	fprintf( header, "const char *uniNamesList_nameAlt(unsigned long uni, unsigned int lang);\n" );
	fprintf( header, "const char *uniNamesList_annotAlt(unsigned long uni, unsigned int lang);\n\n" );
	fprintf( header, "/* Returns 2 lang pointers to names/annotations for this unicode value, */\n" );
	fprintf( header, "/* Return str0=English, and str1=language_version (or NULL if no info). */\n" );
	fprintf( header, "int uniNamesList_nameBoth(unsigned long uni, unsigned int lang, const char **str0, const char **strl);\n" );
	fprintf( header, "int uniNamesList_annotBoth(unsigned long uni, unsigned int lang, const char **str0, const char **str1);\n\n" );
	fprintf( header, "/* Blocklists won't sync if they are different versions. 0=ok, -1=error */\n" );
	fprintf( header, "int uniNamesList_blockCountAlt(unsigned int lang);\n" );
	fprintf( header, "long uniNamesList_blockStartAlt(int uniBlock, unsigned int lang);\n" );
	fprintf( header, "long uniNamesList_blockEndAlt(int uniBlock, unsigned int lang);\n" );
	fprintf( header, "const char *uniNamesList_blockNameAlt(int uniBlock, unsigned int lang);\n" );
	fprintf( header, "int uniNamesList_blockNumberBoth(unsigned long uni, unsigned int lang, int *bn0, int *bn1);\n\n" );
    }

    fprintf( header, "#ifdef __cplusplus\n}\n#endif\n#endif\n" );
    return( 1 );
}

static int dumpblock(FILE *out, FILE *header, int is_fr ) {
    int bcnt, l;
    struct block *block;
    unsigned int i, maxa, maxn;

    l = is_fr; if ( is_fr<0 ) l = 0;

    fprintf( out, "UN_DLL_EXPORT\nconst struct unicode_block UnicodeBlock%s[] = {\n", lg[l] );
    for ( block = head[is_fr], bcnt=0; block!=NULL; block=block->next, ++bcnt ) {
	fprintf( out, "\t{ 0x%x, 0x%x, \"%s\" }%s\n", (unsigned int)(block->start),
		(unsigned int)(block->end), block->name, block->next!=NULL ? "," : "" );
    }
    fprintf( out, "};\n\n" );
    fprintf( header, "/* NOTE: Build your program to access the functions if using multilanguage. */\n\n" );
    if ( is_fr==0 ) fprintf( header, "#define UNICODE_BLOCK_MAX\t%d\n", bcnt );
    fprintf( header, "#define %s\t%d\n", lgb[l], bcnt );
    fprintf( header, "extern const struct unicode_block UnicodeBlock%s[%d];\n", lg[l], bcnt );
    if ( is_fr!=0 ) fprintf( header, "#define UnicodeBlock UnicodeBlock%s\n", lg[l] );

    maxn = maxa = 0;
    for ( i=0; i<sizeof(uniannot[is_fr])/sizeof(uniannot[0][is_fr]); ++i ) {
	if ( uninames[is_fr][i]!=NULL && maxn<strlen(uninames[is_fr][i])) maxn = (unsigned int) strlen(uninames[is_fr][i]);
	if ( uniannot[is_fr][i]!=NULL && maxa<strlen(uniannot[is_fr][i])) maxa = (unsigned int) strlen(uniannot[is_fr][i]);
    }
    if (maxn > max_n ) max_n = maxn;
    if (maxa > max_a ) max_a = maxa;

    fprintf( header, "\n/* NOTE: These %d constants are correct for this version of libuninameslist, */\n", is_fr ? 2: 4 );
    fprintf( header, "/* but can change for later versions of NamesList (use as an example guide) */\n" );
    if ( is_fr==0 ) {
	fprintf( header, "#define UNICODE_NAME_MAX\t%d\n", max_n );
	fprintf( header, "#define UNICODE_ANNOT_MAX\t%d\n", max_a );
	fprintf( header, "#define UNICODE_EN_NAME_MAX\t%d\n", maxn );
	fprintf( header, "#define UNICODE_EN_ANNOT_MAX\t%d\n", maxa );
    }
    if ( is_fr==1 ) {
	fprintf( header, "#define UNICODE_FR_NAME_MAX\t%d\n", maxn );
	fprintf( header, "#define UNICODE_FR_ANNOT_MAX\t%d\n", maxa );
    }
    return( 1 );
}

static int dumparrays(FILE *out, FILE *header, int is_fr ) {
    unsigned int i,j,k,t;
    int l;
    char *prefix = "una";

    l = is_fr; if ( is_fr<0 ) l = 0;

    for ( i=0; i<sizeof(uniannot[0])/(sizeof(uniannot[0][0])*65536); ++i ) {	/* For each plane */
	for ( t=0; t<0xFFFE; ++t )
	    if ( uninames[is_fr][(i<<16)+t]!=NULL || uniannot[is_fr][(i<<16)+t]!=NULL )
	break;
	if ( t==0xFFFE )
    continue;		/* Empty plane */
	for ( j=0; j<256; ++j ) {
	    for ( t=0; t<256; ++t ) {
		if ( uninames[is_fr][(i<<16) + (j<<8) + t]!=NULL || uniannot[is_fr][(i<<16) + (j<<8) + t]!=NULL )
	    break;
		else if ( j==0xff && t==0xfe -1 )
	    break;
	    }
	    if ( t==256 || (j==0xff && t==0xfe -1))
	continue;	/* Empty sub-plane */
	    fprintf( out, "UN_DLL_LOCAL\nstatic const struct unicode_nameannot %s%s_%02X_%02X[] = {\n", prefix, lg[l], i, j );
	    for ( k=0; k<256; ++k ) {
		fprintf( out, "/* %04X */ { ", (i<<16) + (j<<8) + k );
		if ( uninames[is_fr][(i<<16) + (j<<8) + k]==NULL )
		    fprintf( out, "NULL" );
		else
		    dumpstring(uninames[is_fr][(i<<16) + (j<<8) + k],out);
		putc(',',out);
		if ( uniannot[is_fr][(i<<16) + (j<<8) + k]==NULL )
		    fprintf( out, "NULL" );
		else
		    dumpstring(uniannot[is_fr][(i<<16) + (j<<8) + k],out);
		fprintf( out, "}%s\n", k!=255?",":"" );
	    }
	    fprintf( out, "};\n\n" );
	}
    }

    for ( i=0; i<sizeof(uniannot[is_fr])/(sizeof(uniannot[is_fr][0])*65536); ++i ) {	/* For each plane */
	for ( t=0; t<0xFFFE; ++t )
	    if ( uninames[is_fr][(i<<16)+t]!=NULL || uniannot[is_fr][(i<<16)+t]!=NULL )
	break;
	if ( t==0xFFFE )
    continue;		/* Empty plane */
	fprintf( out, "UN_DLL_LOCAL\nstatic const struct unicode_nameannot * const %s%s_%02X[] = {\n", prefix, lg[l], i );
	for ( j=0; j<256; ++j ) {
	    for ( t=0; t<256; ++t ) {
		if ( uninames[is_fr][(i<<16) + (j<<8) + t]!=NULL || uniannot[is_fr][(i<<16) + (j<<8) + t]!=NULL )
	    break;
		else if ( j==0xff && t==0xfe -1 )
	    break;
	    }
	    if ( t==256 )
		fprintf( out, "\tnullarray%s%s\n", lg[l], j!=255?",":"" );
	    else if ( j==0xff && t==0xfe -1 )
		fprintf( out, "\tnullarray2%s\n", lg[l] );
	    else
		fprintf( out, "\t%s%s_%02X_%02X%s\n", prefix, lg[l], i, j, j!=255?",":"" );
	}
	fprintf( out, "};\n\n" );
    }

    fprintf( header, "extern const struct unicode_nameannot * const *const UnicodeNameAnnot%s[];\n", lg[l] );

    fprintf( out, "UN_DLL_EXPORT\nconst struct unicode_nameannot * const *const UnicodeNameAnnot%s[] = {\n", lg[l] );
    for ( i=0; i<sizeof(uniannot[is_fr])/(sizeof(uniannot[is_fr][0])*65536); ++i ) {	/* For each plane */
	for ( t=0; t<0xFFFE; ++t )
	    if ( uninames[is_fr][(i<<16)+t]!=NULL || uniannot[is_fr][(i<<16)+t]!=NULL )
	break;
	if ( t==0xFFFE )
	    fprintf( out, "\tnullnullarray%s,\n", lg[l] );
	else
	    fprintf( out, "\t%s%s_%02X,\n", prefix, lg[l], i );
    }
    while ( i<0x20 ) {
	fprintf( out, "\tnullnullarray%s%s\n", lg[l], i!=0x20-1?",":"" );
	++i;
    }
    fprintf( out, "};\n\n" );
    return( 1 );
}

static int dump(int is_fr) {
    int dumpOK=0;

    FILE *out = fopen(is_fr ? "nameslist-fr.c":"nameslist.c","w");
    if ( out==NULL ) {
	fprintf( stderr, "Cannot open output file\n" );
	return( dumpOK );
    }
    FILE *header = fopen( is_fr ? "uninameslist-fr.h": "uninameslist.h","w");
    if ( header==NULL ) {
	fprintf( stderr, "Cannot open output header file\n" );
	fclose(out);
	return( dumpOK );
    }

    if ( dumpinit(out,header,is_fr) && dumpblock(out,header,is_fr) && \
	 dumparrays(out,header,is_fr) && dumpend(header,is_fr) && \
	 fflush(out)==0 && fflush(header)==0 )
	dumpOK=1;
    fclose(out); fclose(header);
    return( dumpOK );
}

int main(int argc, char **argv) {
    int errCode=1;

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);
    InitArrays();
    max_a = max_n = 0;
    if ( ReadNamesList() && dump(1/*french*/) && dump(0/*english*/) )
	errCode=0;
    FreeArrays();
    return( errCode );
}
