/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "stt/MakeSteinerTreeBuilder.h"

#include "stt/SteinerTreeBuilder.h"
#include "stt/flute.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* stt_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Stt_Init(Tcl_Interp* interp);
}

namespace ord {

stt::SteinerTreeBuilder* makeSteinerTreeBuilder()
{
  return new stt::SteinerTreeBuilder();
}

void deleteSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder)
{
  stt::flt::deleteLUT();
  delete stt_builder;
}

void initSteinerTreeBuilder(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Stt_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::stt_tcl_inits);
  openroad->getSteinerTreeBuilder()->init(openroad->getDb(), openroad->getLogger());
  stt::flt::readLUT();
}

}  // namespace ord
