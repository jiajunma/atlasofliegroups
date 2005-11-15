/*
  This is main.c

  Copyright (c) 2004 Fokko du Cloux

  part of the Atlas of Reductive Lie Groups project

  This program is made available under the terms stated in the Gnu
  General Public License below. Enquiries about the General Public License
  and the Gnu project may be adressed to :

      Free Software Foundation
      675 Mass. Ave., Cambridge, MA 02139, USA

  		    GNU GENERAL PUBLIC LICENSE
     TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

    0. This License Agreement applies to any program or other work which
  contains a notice placed by the copyright holder saying it may be
  distributed under the terms of this General Public License.  The
  "Program", below, refers to any such program or work, and a "work based
  on the Program" means either the Program or any work containing the
  Program or a portion of it, either verbatim or with modifications.  Each
  licensee is addressed as "you".

    1. You may copy and distribute verbatim copies of the Program's source
  code as you receive it, in any medium, provided that you conspicuously and
  appropriately publish on each copy an appropriate copyright notice and
  disclaimer of warranty; keep intact all the notices that refer to this
  General Public License and to the absence of any warranty; and give any
  other recipients of the Program a copy of this General Public License
  along with the Program.  You may charge a fee for the physical act of
  transferring a copy.

    2. You may modify your copy or copies of the Program or any portion of
  it, and copy and distribute such modifications under the terms of Paragraph
  1 above, provided that you also do the following:

      a) cause the modified files to carry prominent notices stating that
      you changed the files and the date of any change; and

      b) cause the whole of any work that you distribute or publish, that
      in whole or in part contains the Program or any part thereof, either
      with or without modifications, to be licensed at no charge to all
      third parties under the terms of this General Public License (except
      that you may choose to grant warranty protection to some or all
      third parties, at your option).

      c) If the modified program normally reads commands interactively when
      run, you must cause it, when started running for such interactive use
      in the simplest and most usual way, to print or display an
      announcement including an appropriate copyright notice and a notice
      that there is no warranty (or else, saying that you provide a
      warranty) and that users may redistribute the program under these
      conditions, and telling the user how to view a copy of this General
      Public License.

      d) You may charge a fee for the physical act of transferring a
      copy, and you may at your option offer warranty protection in
      exchange for a fee.

  Mere aggregation of another independent work with the Program (or its
  derivative) on a volume of a storage or distribution medium does not bring
  the other work under the scope of these terms.

    3. You may copy and distribute the Program (or a portion or derivative of
  it, under Paragraph 2) in object code or executable form under the terms of
  Paragraphs 1 and 2 above provided that you also do one of the following:

      a) accompany it with the complete corresponding machine-readable
      source code, which must be distributed under the terms of
      Paragraphs 1 and 2 above; or,

      b) accompany it with a written offer, valid for at least three
      years, to give any third party free (except for a nominal charge
      for the cost of distribution) a complete machine-readable copy of the
      corresponding source code, to be distributed under the terms of
      Paragraphs 1 and 2 above; or,

      c) accompany it with the information you received as to where the
      corresponding source code may be obtained.  (This alternative is
      allowed only for noncommercial distribution and only if you
      received the program in object code or executable form alone.)

  Source code for a work means the preferred form of the work for making
  modifications to it.  For an executable file, complete source code means
  all the source code for all modules it contains; but, as a special
  exception, it need not include source code for modules which are standard
  libraries that accompany the operating system on which the executable
  file runs, or for standard header files or definitions files that
  accompany that operating system.

    4. You may not copy, modify, sublicense, distribute or transfer the
  Program except as expressly provided under this General Public License.
  Any attempt otherwise to copy, modify, sublicense, distribute or transfer
  the Program is void, and will automatically terminate your rights to use
  the Program under this License.  However, parties who have received
  copies, or rights to use copies, from you under this General Public
  License will not have their licenses terminated so long as such parties
  remain in full compliance.

    5. By copying, distributing or modifying the Program (or any work based
  on the Program) you indicate your acceptance of this license to do so,
  and all its terms and conditions.

    6. Each time you redistribute the Program (or any work based on the
  Program), the recipient automatically receives a license from the original
  licensor to copy, distribute or modify the Program subject to these
  terms and conditions.  You may not impose any further restrictions on the
  recipients' exercise of the rights granted herein.

    7. The Free Software Foundation may publish revised and/or new versions
  of the General Public License from time to time.  Such new versions will
  be similar in spirit to the present version, but may differ in detail to
  address new problems or concerns.

  Each version is given a distinguishing version number.  If the Program
  specifies a version number of the license which applies to it and "any
  later version", you have the option of following the terms and conditions
  either of that version or of any later version published by the Free
  Software Foundation.  If the Program does not specify a version number of
  the license, you may choose any version ever published by the Free Software
  Foundation.

    8. If you wish to incorporate parts of the Program into other free
  programs whose distribution conditions are different, write to the author
  to ask for permission.  For software which is copyrighted by the Free
  Software Foundation, write to the Free Software Foundation; we sometimes
  make exceptions for this.  Our decision will be guided by the two goals
  of preserving the free status of all derivatives of our free software and
  of promoting the sharing and reuse of software generally.

  			    NO WARRANTY

    9. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
  FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
  OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
  PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
  OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
  TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
  PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
  REPAIR OR CORRECTION.

    10. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
  WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
  REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
  INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
  OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
  TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
  YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
  PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGES.

  		     END OF TERMS AND CONDITIONS
*/

#include <iostream>

#include "commands.h"
#include "constants.h"
#include "emptymode.h"
#include "error.h"
#include "input.h"
#include "pool.h"

int main(int argc, char* argv[])

/*
  For now, we do nothing with the arguments.
*/

  
try

{
  using namespace atlas;
  using namespace pool;

  constants::initConstants();
  input::initReadLine();
  commands::run(emptymode::emptyMode());

  pool::memoryReport();

  return 0;
}

catch (atlas::error::NumericOverflow& e) {

  std::cerr << "error: uncaught NumericOverflow" << std::endl;

}

catch (atlas::error::NumericUnderflow& e) {

  std::cerr << "error: uncaught NumericUnderflow" << std::endl;

}

catch (...) {

  std::cerr << "error: uncaught exceptions on exit" << std::endl;

}
