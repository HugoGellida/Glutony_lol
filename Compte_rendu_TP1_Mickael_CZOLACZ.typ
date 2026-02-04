/* GPTOUM !!!*/
#set page(
  margin: (top: 2.2cm, bottom: 2.2cm, left: 2.2cm, right: 2.2cm),
)


#let nom = "CZOLACZ"
#let prenom = "Mickaël"
#let classe = "Imagine - M1"
#let annee = "2025–2026"

#let titre = "Compte rendu de TP"
#let sous_titre = "[HAI819I] Moteur de jeu - TP1/TP2"

/* HEADER */
#align(top)[
  #grid(
    columns: (1fr, 1fr),
    gutter: 0pt,
    [
      *#nom* #prenom \
      #classe
    ],
    [
      #align(right)[
        #annee
      ]
    ],
  )
  #v(0.4cm)
  #line(length: 100%)
]

/* CENTRE DE PAGE */
#v(5cm)

#align(center)[
  #text(size: 28pt, weight: "bold")[#titre]
  #v(0.4cm)
  #text(size: 16pt)[#sous_titre]
]

/* (Optionnel) footer / infos en bas
#align(bottom + center)[
  #v(8cm)
  #text(size: 10pt, fill: gray)[Université / Prof / etc.]
]
*/
/* FIN GPTOUM */



/* Sommaire et numbering
#set page(numbering :"1")
#show link : underline
#show link : set text(fill : blue)

#pagebreak()
#outline(target:heading, title: "Table de matières", depth:3)
*/
#let cite(target) = eval("@"+target+" (#ref(<"+target+">, form:\"page\"))", mode:"markup")     

// YOUPI !!! J'AI ENFIN REUSSI
#let view(items, label) = figure(
  rect(
    table(
      columns: items.len(),
      inset: 0pt,
      align: center,
      stroke: none,
      ..items.map(item => { // items comme un stream je crois, sinon on peut faire un for (à la c++/java mais iter python) si on veut une liste
        
        let (elem, cap) = item
        figure(
          scale(elem, 100%, reflow: true),
          caption: scale(cap, 100%, reflow: true),
          numbering: none,
        )
      }),
    )
  ),
  caption: label,
  supplement: [Vue]
)





#set par(
  first-line-indent: (amount : 15pt, all:true),
  spacing: 0.65em,
  justify: true,
)

#let cross(a, b) = $arrow(a) times arrow(b)$

#let mulCompWise(A, B) = eval("$vec("+A+"_r times "+B+"_r,"+A+"_g times "+B+"_g,"+A+"_b times "+B+"_b)$")

#let normalize(content) = $content / norm(content)$

#let file(target) = underline(text(target, color.blue.darken(50%)))

#pagebreak()
#show ref : underline


#set heading(numbering: "I.1.a)")
=
==
  Avant tout, en base, j'utilise un ensemble de classes que j'ai écrites pour un projet personel, Glutony, initialement basé sur glut, mais réécrit pour GLFW et glm. La base de code fournie à donc été prèsque entièrement retirée, car déjà écrite dans glutony.

  Mon code utilise des scenes contant des arborescences de GameObject, dont les transformations sont appliquées relativement au parent(Comme Unity). Chaque gameObject à une liste de composants, qui peuvent êtres ajouté et retiré au runtime. Par exemple, les composants meshRenderer et mesh, qui permettent à un object d'avoir un maillage et une passe de rendu avec un shader.

  Petite précision sur le meshRenderer, il utilise un materiau qui est une classe de stockage d'uniforms, qui dépend d'un shader, et qui se charge de syncroniser les uniforms qu'il contient. Un materiau ne fonctionne que sur le shader pour lequel il est créé, mais un même shader peut être utilisé par plusieurs matériaux différents (comme Unity).

  Pour le plan, on construit comme demandé, puis il est affiché:

  #rect([#figure(image("imgRapport/plane.png"),caption:"Plan demandé en question 1, fov:60°")])

  La touche g permet d'activer des controles FPS [Z/Q/S/D] [E:UP] [A:DOWN].
  La direction de mouvement est relative à la rotation sur l'axe Y seulement, pour contraindre horizontalement.

  MeshRenderer supporte les uvs, si le composant Mesh en déclare, il les envois sur l'attribut 2 du VAO. En #ref(<view2>) nous pouvons voir les coordonnées de textures affichées telle que $"col" = vec("uv"_u, "uv"_v, 0.4)$

  #rect([#figure(image("imgRapport/plane_UV.png"), caption:"Plan avec les coordonnées de textures") <view2>])

==
  Pour l'altitude, un bruit aléatoire continu et cohérent me semblait plus utile (et cela servira surement plus tard) donc j'ai écrit un composant MeshNoiseDeformPerlinHeight, qui applique un bruit de Perlin(3D) à la hauteur d'un maillage, en fonction des coordonnées objet de ses vertices.
  la #ref(<view3>) montre le resultat, avec une texture en plus.
  #rect([#figure(image("imgRapport/perlin.png"), caption: "plan texturé avec un bruit de Perlin")<view3>]) 

=
  