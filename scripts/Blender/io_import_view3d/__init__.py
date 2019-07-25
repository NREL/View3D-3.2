# View3D, Copyright (c) 2018 Alliance for Sustainable Energy, LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. The name of the copyright holder(s), any contributors, the United States
#    Government, the United States Department of Energy, or any of their
#    employees may not be used to endorse or promote products derived from this
#    software without specific prior written permission from the respective party.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND ANY CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER(S), ANY CONTRIBUTORS, THE
# UNITED STATES GOVERNMENT, OR THE UNITED STATES DEPARTMENT OF ENERGY, NOR ANY OF
# THEIR EMPLOYEES, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.

text = '''
T Pinney & Bean test 4: L shaped room (page A2.14; Table A2.11) BRE
C  encl=1 list=2 eps=1.e-4 maxu=8 maxo=8 mino=0 emit=1
F  3
!  #   x    y    z      coordinates of vertices
V  1   0.   3.   3.
V  2   1.   3.   3.
V  3   0.   3.   0.
V  4   1.   3.   0.
V  5   0.   1.   0.
V  6   1.   1.   0.
V  7   3.   1.   0.
V  8   0.   0.   0.
V  9   1.   0.   0.
V 10   3.   0.   0.
V 11   0.   0.   3.
V 12   1.   0.   3.
V 13   3.   0.   3.
V 14   0.   1.   3.
V 15   1.   1.   3.
V 16   3.   1.   3.
!  #   v1  v2  v3  v4 base cmb  emit  name      surface data
S  1   11  13  10   8   0   0   0.90  srf-1
S  2   10  13  16   7   0   0   0.90  srf-2
S  3    6   7  16  15   0   0   0.90  srf-3
S  4    4   6  15   2   0   0   0.90  srf-4
S  5    3   4   2   1   0   0   0.90  srf-5
S  6    8   3   1  11   0   0   0.90  srf-6
S  7   11  14  15  12   0   0   0.90  srf-7
S  8    8   9   6   5   0   0   0.90  srf-8
S  9    1   2  15  14   0   7   0.90  srf-7b    ! surfaces that combine
S 10   15  16  13  12   0   7   0.90  srf-7c
S 11    5   6   4   3   0   8   0.90  srf-8b
S 12    9  10   7   6   0   8   0.90  srf-8c
End of data
'''

bl_info = {
    "name": "Import View3D Geometry",
    "category": "Object",
}

import bpy
import bmesh

import os
import sys
dir = os.path.dirname(bpy.data.filepath)
if not dir in sys.path:
    sys.path.insert(0,dir)
import v3d

class ImportView3D(v3d.VS3, bpy.types.Operator):
    """Import View3D Geometry"""      # blender will use this as a tooltip for menu items and buttons.
    bl_idname = "object.view3d_importer"        # unique identifier for buttons and menu items to reference.
    bl_label = "Import View3D Geometry"         # display name in the interface.
    bl_options = {'REGISTER', 'UNDO'}  # enable undo for the operator.

    def __init__(self):
        super().__init__()
        #raise Exception('"%s"' % sys.path)
        #raise Exception(os.path.realpath(__file__))

    def execute(self, context):
        self.read(text.splitlines())

        # Figure out if anything went wrong
        if len(self.errors) > 0:
            self.report({'ERROR'}, 'Errors in input:\n%s' % '\n'.join(self.errors))
            return {'CANCELLED'}
        
        # Create geometry
        for surface in self.surfaces:
            bm = bmesh.new()
            for vertex in surface.vertices:
                bm.verts.new((vertex.x, vertex.y, vertex.z))
            
            bm.faces.new(bm.verts)
            bm.normal_update()
                
            me = bpy.data.meshes.new("")
            bm.to_mesh(me)
            ob = bpy.data.objects.new("", me)
            context.scene.objects.link(ob)
            context.scene.update()

        return {'FINISHED'}

def register():
    bpy.utils.register_class(ImportView3D)


def unregister():
    bpy.utils.unregister_class(ImportView3D)


# This allows you to run the script directly from blenders text editor
# to test the addon without having to install it.
if __name__ == "__main__":
    register()
