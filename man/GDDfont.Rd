\name{GDDfont}
\alias{.GDD.font}
\alias{GDD.fonts}
\title{
  Font handling in GDD
}
\description{
  \code{.GDD.font} shows the current font mapping or performs a font look-up.
}
\usage{
.GDD.font(name = NULL)
}
\arguments{
  \item{name}{If this parameter is \code{NULL} then the current mapping
  table is returned. Otherwise this parameter specifies the name of the font
  to look up.}
}
\value{
  If the \code{name} is \code{NULL} then the return value is a vector of strings. Each entry is a path to the font file or \code{NA} denoting that such font could not be found. First entry corresponds to the font 1 (plain), second to 2 (bold), etc. (see \code{font} parameter of \code{\link{par}} for details).
}
\section{Font handling}{
  \code{GDD} uses FreeType library for drawing text where available.
  However, FreeType support of GD requires full path to the font file
  to be specified. Therefore GDD maps R font IDs into file paths. This
  is done using the \code{basefont.config} file in the \code{fonts}
  directory of the GDD package. That file defines which files will be used
  by the GDD device.
  
  The supported entries are "base.norm" for regular base font, "base.bold"
  for bold base font, "base.ital" for italic base font and "base.bita"
  for bold, italic base font. Since 0.1-8 you can also specify the
  symbol font using "symbol" entry (see the default
  \code{basefont.mapping} file for examples). The mapping file is processed
  in a greedy manner, i.e. the first match is used.

  Some systems have the fontconfig library which makes finding font files
  somewhat easier. On such systems the actual font specification in the
  mappings file can be of the form \code{<Font>} instead of the path,
  e.g. \code{<Arial:bold>}.

  The \code{.GDD.font} function can be used to perform fontconfig lookup
  or to return the currently used font files. If a font name is specified
  as the \code{name} parameter, the corresponding font file is located.
  If there is no fontconfig library or the lookup was unsuccessful,
  \code{NULL} is returned.

  If the \code{name} parameter is set to \code{NULL}, then \code{.GDD.font}
  simply returns the list of font files currently used.
} 
\seealso{
}
\examples{
# return currently used fonts
.GDD.font()

# look up font closest to Times
# (returns non-NULL only if fontconfig support is enabled
# in libgd and some Times-like fonts exists)
.GDD.font("Times")
}
\keyword{device}
