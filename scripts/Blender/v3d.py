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

class BadInput(Exception):
    pass

class SurfaceProperties:
    def __init__(self, name, index, vertices, emissivity, base=None, combine=None):
        self.name = name
        self.index = index
        self.vertices = vertices
        self.emissivity = emissivity
        self.base = base
        self.combine = combine

class VertexProperties:
    def __init__(self, index, x, y, z):
        self.index = index
        self.x = x
        self.y = y
        self.z = z

class CalculationControl:
    def __init__(self, eps=1.0e-4, maxU=12, maxO=8, minO=0, row=0, col=0, encl=0,
                 emit=0, out=0, list=0):
        self.eps = eps
        self.maxU = maxU
        self.maxO = maxO
        self.minO = minO
        self.row = row
        self.col = col
        self.encl = encl
        self.emit = emit
        self.out = out
        self.list = list

class VS3:
    """Read/Write View3D Geometry"""
    def __init__(self):
        self.clear()
        self.line_number = 0
        
    def clear(self):
        self.vertices = []
        self.surfaces = []
        self.errors = []
        self.format = None
        self.title = ''
        self.control = {}

    def read(self, fp):
        # Process the input
        self.clear()
        self.line_number = 1
        for line in fp:
            line = line.strip()
            try:
                if line:
                    c = line[0].upper()
                    if c == 'F':
                        self.format = line[1:].strip()
                        if self.format != '3':
                            self.errors.append('Error on line %d: Unsupported format "%s"' % (self.line_number, self.format))
                            return False
                    elif c == 'T':
                        self.title = line[1:].strip()
                    elif c == 'V':
                        self.vertices.append(self.string_to_vertex(line))
                    elif c == 'S':
                        self.surfaces.append(self.string_to_surface(line))
                    elif c == '!':
                        pass
                    elif c == 'C':
                        self.control['all'] = line[1:].strip()
                    elif c == 'E':
                        break
                    else:
                        self.errors.append('Error on line %d: Unrecognized input type "%s"' % (self.line_number, line[0]))
            except BadInput as e:
                self.errors.append(str(e))
            
            self.line_number += 1
        # Figure out if anything went wrong
        if len(self.errors) > 0:
            return False
        
        # Check for consistent numbering in the vertices and surfaces
        for i,vertex in enumerate(self.vertices, 1):
            if i != vertex.index:
                self.errors.append('Error in input vertex %d: Index "%d" does not match position.' % (i, vertex.index))
                return False
        for i,surface in enumerate(self.surfaces, 1):
            if i != surface.index:
                self.errors.append('Error in input surface %d: Index "%d" does not match position.' % (i, vertex.index))
                return False

        # Make internal connections between the surfaces and the vertices
        for surface in self.surfaces:
            links = []
            for vertex_id in surface.vertices:
                if vertex_id < 0:
                    self.errors.append('Error in input surface %d: Negative vertex index "%d".' % (surface.index, vertex_id))
                    links.append(None)
                elif vertex_id > len(self.vertices):
                    self.errors.append('Error in input surface %d: Vertex index "%d" too large.' % (surface.index, vertex_id))
                    links.append(None)
                else:
                    links.append(self.vertices[vertex_id-1])
            surface.vertices = links
        if len(self.errors) > 0:
            return False

        # Make internal connections between base and child surfaces
        for surface in self.surfaces:
            base = None
            if surface.base:
                if surface.base >= surface.index:
                    self.errors.append('Error in input surface %d: Base surface index "%d" must be less than surface index.' % (surface.index, surface.base))
                elif surface.base < 0:
                    self.errors.append('Error in input surface %d: Negative base surface index "%d".' % (surface.index, surface.base))
                else:
                    base = self.surfaces[surface.base-1]              
            surface.base = base
        if len(self.errors) > 0:
            return False

        # Make internal connections between combination surfaces
        for surface in self.surfaces:
            combine = None
            if surface.combine:
                if surface.combine >= surface.index:
                    self.errors.append('Error in input surface %d: Combination surface index "%d" must be less than surface index.' % (surface.index, surface.combine))
                elif surface.combine < 0:
                    self.errors.append('Error in input surface %d: Negative base surface index "%d".' % (surface.index, surface.combine))
                else:
                    combine = self.surfaces[surface.combine-1]
            surface.combine = combine
        if len(self.errors) > 0:
            return False
        
        return True

    def string_to_vertex(self, line):
        # Assume string has already been stripped, first char is 'V'
        data = line[1:].strip().split()
        if len(data) < 4:
            raise BadInput('On line %d: Insufficient vertex data' % self.line_number)
        elif len(data) > 4:
            # This should be a warning
            pass
        index = self.convert_integer(data[0], 'Non-integral vertex number "%s"')
        x = self.convert_float(data[1], 'Non-numeric x coordinate "%s"')
        y = self.convert_float(data[2], 'Non-numeric y coordinate "%s"')
        z = self.convert_float(data[3], 'Non-numeric z coordinate "%s"')
        return VertexProperties(index, x, y, z)

    def string_to_surface(self, line):
        # Assume string has already been stripped, first char is 'S'
        data = line[1:].strip().split()
        if len(data) < 9:
            raise BadInput('On line %d: Insufficient surface data' % self.line_number)
        elif len(data) > 9:
            # Have data, should use it for something
            pass
        index = self.convert_integer(data[0], 'Non-integral surface number "%s"')
        v1 = self.convert_integer(data[1], 'Non-integral vertex 1 number "%s"')
        v2 = self.convert_integer(data[2], 'Non-integral vertex 2 number "%s"')
        v3 = self.convert_integer(data[3], 'Non-integral vertex 3 number "%s"')
        v4 = self.convert_integer(data[4], 'Non-integral vertex 4 number "%s"')
        base = self.convert_integer(data[5], 'Non-integral base surface number "%s"')
        combo = self.convert_integer(data[6], 'Non-integral combination surface number "%s"')
        e = self.convert_float(data[7], 'Non-numeric emissivity "%s"')
        name = data[8]
        return SurfaceProperties(name, index, [v1, v2, v3, v4], e, base=base, combine=combo)

    def string_to_control(self, line):
        # Assume string has already been stripped, first char is 'S'
        data = line[1:].strip().split()
        left,eq,right = data.partition('=')
        

    def convert_integer(self, str, mesg):
        try:
            return int(str)
        except ValueError:
            raise BadInput('Error on line %d: %s' % (self.line_number, mesg % str))

    def convert_float(self, str, mesg):
        try:
            return float(str)
        except ValueError:
            raise BadInput('Error on line %d: %s' % (self.line_number, mesg % str))

if __name__ == "__main__":
    vs3 = VS3()
    print(vs3.read(text.splitlines()))
