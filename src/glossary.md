# Glossary

Term        | Meaning
------------|------------
*glyph*     | ¹
*em unit*   | ¹
*FUnit*     | ¹
*outline*   | a transform / resolution independent visual representation of a glyph
*curve*     |	a quadratic bezier curve
*line*      | a simple, straight line
*dot*       | a short line that is entirely contained within one pixel (think 'dotted line')
*point*     | a point in 2d space, where one unit equals one pixel
*cover*     | the height that a particular dot span within one pixel ²
*area*      | the signed area between a line and the left edge of the raster ²
*transform* |
*parse*     | *Noun*: The result of parsing. *Verb*: Converting raw TrueType data into an outline
*tessel*    | short for tesselation / tesselate.
*raster*    | *Noun*: A representation of the outline that is transform-specific. *Verb*: turning a tesselation into a raster
*gather*    | *Verb*: generating a finalized image from a raster

¹ this term is commonly used in general typography or is described in the TrueType / OpenType specification.</br>
² this term was inherited from Anti-Grain Geometry and cl-vectors.
