Runner for Windows
==================

This program (RUN) lets a Windows user easily run programs wherever they are on the disk. The program requires a local installation of the awesome VoidTools program Everything Search - that program is available for free download at http://www.voidtools.com/download.php

For those of you who occasionally need to use the command line or who need to run program specifying command line options, there is a common annoyance that in many cases, the program is not on the system path so we need to open a command box, locate the program, change directory to where it is (or add it to the path) and only then we can run it. RUN eliminates this annoyance with EverythingSearch's magic.

Usage
-----
    run [options] <program> <...program parameters...>
    program:	(Partial) name of program without .exe
    -l		Just list matching names
    -#		Force the use of the #'th program (as shown with -l)
    -s		With -# saves the #'th program as favorite (as shown with -l)
    -d		Remove the favorite program specified
    -f		List favorite programs
    -w		Use whole-word search
    -p		Pause after run

Example
-------

    $ run *word mydoc.docx
    Running: C:\Program Files\Microsoft Office 15\root\office15\WINWORD.EXE:

    $ run -l *word
    1) CHOSEN: WINWORD.EXE [C:\Program Files\Microsoft Office 15\root\office15]
    2) FREEWORD.EXE [C:\Program Files (x86)\Freeware Inc]
    3) CROSSWORD.EXE [C:\Program Files\Thoughtful Diversions]

    $ run -3 *word level.txt
    Running: C:\Program Files\Thoughtful Diversions\CROSSWORD.EXE:

    $ run -3 -s *word level.txt
    Saved favorite 'word' as: C:\Program Files\Thoughtful Diversions\CROSSWORD.EXE
    
    $ run word level.txt
    Running: C:\Program Files\Thoughtful Diversions\CROSSWORD.EXE:

Building
--------
1. Make sure CMake is installed (e.g. "choco install cmake")
2. Run build.bat

	
Author
------
Dror Harari

Changes
-------
* 2022-01-29 V5
  1. Switch to Everything 1.4 SDK
  2. Switch to CMake build (tested with Visual Studio 2019)
* 2017-11-05 V4
  1. Empty parameter on the command line (given as "") were previously removed. 
     Fixed - now if "" is specified in the Run command line, it is passed to the
     program that is run.
  2. Make the message given when Everything Search is not running more meaningful.
  3. Switch to building with Visual Studio 2010.
* 2015-04-11 V3
  1. Added the -s option to save favorite the program location
  2. Added the -d option to delete saved favorite program location
  3. Added the -f option to list saved favorite program locations
  4. Bug fixes
* 2015-01-18	V2
  1. Added the -p option
  2. Filter out some file with unuseful suffixes (.pf, .res, .mui)
  3. Filter out files from the windows\prefetch directory


Copyrights & Disclaimers
------------------------
All the files named Everything.* where taken from VoidTools' Everything 
Search SDK, The other files were created by me and are hereby placed in 
the public domain

DISCLAIMER (Cover my A**)
-------------------------

THIS SOFTWARE, 'RUN' AVAILABLE HEREBY IS PROVIDED "AS IS" AND ANY EXPRESSED 
OR IMPLIEd WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN 
NO EVENT SHALL THE AUTHOR, OR ANY CONTRIBUTOR BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
