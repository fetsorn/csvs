# The Forest and The Garden
so the idea is to write a book in the style of Rust and org-mode manual use the terminology but make no reference to csvs and evenor. one can write the same with other tools, but still have leaves, queries, filters and overviews. the design of a consumer dataset is convergent.

speak of security, independence and control, discuss data models for common domains and low level methods like sow and mow. discuss big notation without mention of tools. talk of motivations and aspirations but make no mentions of competitors or status quo.

talk as if the future is in the present and warrants a guide for the layman of the past

guide for the qualitative self

talk about how we all tend to our gardens together.

find analog precursors to the data models.

can dogs grow trees? monkeys can with sign boards. dogs remember their men. can worms grow trees? they remember the model of their body and the directions around. can rocks grow trees? they remember air pressure, atomic structure, history of change. can elementary particles grow trees? they remember environment and forces. can mystery grow trees? it can.

ultimately it is a matter of awareness, of certifying senses and building upon.

you wake up… you come to your senses… a root clings, a root sprouts, a branch grows, a leaf ties to a trunk, a twig halts, for now…

tending to the garden of your awareness

it is about empowerment. people can remember for longer, and think about memories stronger, remember more. it is about emancipation. people can tie away from alien narrators and tell the story of their own data.
it is about guidance. people can go forth into the forest, farther, light up the mystery, further, braver, and tend to the garden longer.

tend to the garden with tools


we map domains into schemas, we get the garden fully formed we read, mow, and we write, sow, we tend to the trees.

the forest and the garden. out there, in the wild, is great mystery. we sense it, and make a garden in its image.

talk about the withers, the weaknesses of a garden.

talk about the haunts, the perils of the forest.

and how we find them, how we feel about them, how we fear them, and how we fight them.

mapping a schema is the chief proposition of the whole thing.

some time the schema will find a name in the spirit of trees.

it’s like the shape, morphology, and like the gene, taxonomy, and like the kin, provenance.

it’s like a simplified shape, a form in the sense of the cast or the mould. 

a kind of seed bed to match the seed, and a kind of cover to match the crown.

it’s like the shadow and the smell.

schema depends on perspective and the environment.

talk about trunks and branches

it stems from tools and methods

don’t talk about string typed data

my environment is in my awareness but i refuse to tend to that as a garden. i say tools will come just like always. 

almost like a treatise on gardening. first we talk about a foreword to set the tone, then we give an example to get people started, then we talk about scheming, and making schemas for domains. then we talk about crud operations on various domains and we can organize that part organically

separate search from tending

 table of contents
 - foreword
 - introduction
 - getting started
 - scheming
 - project
 - tending
 - project
 - domains
 - reference

navigation should go to the advanced reference

## do not talk
do not talk about status quo about how memories fall to oblivion, or trees without water.
do not talk about status quo how we hold that garden in mind and in speech, and objects, and habits, and society, losing it to space and time
do not talk about other tools, do not talk about the market
talk of ways, do not talk of lands. obscure talking about the great mystery, just say “mystery”.
do not talk about schema-less garden of mind
do not talk about forced schemas of corporate gardens, monoculture, rigid, narrow. but talk about keeping the garden wide and open.
do not talk about history. empower, don’t enlighten
do not talk about similarities with the method of loci

## view
data inside

### list of texts
have some text

```
went to the zoo today, stayed home yesterday
```

### graph of tokens
separate text into tokens that are connected to each other

```
went to the zoo-today
stayed home-yesterday
```

### cluster of entities
group some tokens into clusters and name each cluster as an entity

```
| event            | day       |
| ---------------- | --------  |
| went to the zoo  | today     |
| stayed home      | yesterday |
```

### tree of entries
choose one entity as a root and cut connections to it

```
.
|
|-- went to the zoo
| |--- today
|-- stayed at home
  |--- yesterday
```

## dataset
shorthand data inside

### list of texts
have some text

```js
{ _: "dataset", 
  text: ["went to the zoo today", "stayed home yesterday"] }
```

### graph of tokens
separate text into tokens that are connected to each other

```js
{ _: "dataset", 
  token: [
    { _: "token", 
      token: "today", 
      relation: "went to the zoo" }
    { _: "token", 
      token: "went to the zoo", 
      relation: "today" }
    { _: "token", 
      token: "yesterday", 
      relation: "stayed home" }
    { _: "token", 
      token: "stayed home", 
      relation: "yesterday" }
  ] }
```

### cluster of entities
group some tokens into clusters and name each cluster as an entity

```js
{ _: "dataset", 
  event: [
    { _: "event", 
      event: "went to the zoo", 
      day: "today" }, 
    { _: "event", 
      event: "stayed at home", 
      day: "yesterday" }
  ], 
  day: [
     { _: "day", 
       day: "today", 
       event: "went to the zoo" }, 
     { _: "day", 
       day: "yesterday", 
       event: "stayed at home" }
   ] }
```

### tree of entries
choose one entity as a root and cut connections to it

```js
{ _: "dataset", 
  event: [ 
    { _: "event", event: "went to the zoo", day: "today" }, 
    { _: "event", event: "stayed at home", day: "yesterday" }
  ], 
  day: [
    { _: "day", day: "today" }, 
    { _: "day", day: "yesterday" }
  ] }
```

## branch
structure of data inside

describe each level as a tree where a possible entity is called "branch", it connects to "leaves" and leaves connect to "trunk".

```js
{ _: "_", 
  root: "branch", 
  branch: ["trunk", "leaf"] }
```

### list of texts
have some dataset with text

```js
{ _: "root", 
  branch: [
    { _: "branch",
      branch: "dataset",
      trunk: [],
      leaf: ["text"] }
    
    { _: "branch",
      branch: "text",
      trunk: ["dataset"],
      leaf: [] }
  ] }
```

### graph of tokens
separate text into tokens, each token has a relation to some other token

```js
{ _: "root", 
  branch: [
    { _: "branch",
      branch: "dataset",
      trunk: [],
      leaf: ["token"] }
    
    { _: "branch",
      branch: "token",
      trunk: ["dataset"],
      leaf: ["relation"] }
    
    { _: "branch",
      branch: "relation",
      trunk: ["token"],
      leaf: [] }
  ] } 
```

### cluster of entities
group tokens into clusters "event" and "day"

```js
{ _: "root", 
  branch: [
    { _: "branch", 
      branch: "dataset", 
      trunk: [], 
      leaf: ["event", "day"] }
    
    { _: "branch", 
      branch: "event", 
      trunk: ["dataset", "day"], 
      leaf: ["day"] } 
    
    { _: "branch", 
      branch: "day",
      trunk: ["dataset", "event"], 
      leaf: ["event"] }
  ] }
```

### tree of entries
choose the "event" entity as root and cut connection to it from "day"

```js
{ _: "root", 
  branch: [
    { _: "branch",
      branch: "dataset",
      trunk: [],
      leaf: ["event"] }
    
    { _: "branch",
      branch: "event",
      trunk: ["dataset"],
      leaf: ["day"] }
    
    { _: "branch",
      branch: "day",
      trunk: ["event"],
      leaf: [] }
  ] }
```
  
## schema
shorthand structure of data inside

write trunk as field name and its leaves as a list of values

### list of texts
have some text
  
```js 
{ _: "_", dataset: ["text"], text: [] }
```
### graph of tokens
separate text into tokens that are related to each other

```js 
{ _: "_", dataset: ["token"], token: ["relation"], relation: [] }
```

### cluster of entities
group tokens into clusters "event" and "day"


```js
{ _: "_", 
  dataset: ["event", "day"], 
  event: ["day"], 
  day: ["event"] }
```

### tree of entries
choose the "event" entity as root and cut connection to it from day

```js 
{ _: "_", dataset: ["event"], event: ["day"], day: [] }
```

## crud api

add new branch, remove branch, remove branch and its crown, remove branch and graft its crown to trunk, rewrite entire tree, find leaves, find trunks, find roots, find cognates, find laterals, find neighbours, find twigs, graft one branch to another, download tree, upload tree, find number of branches, find crown from root, find crown from twig, find all ancestors of a branch, find all progeny of a branch, insert branch with leaves to a branch, turn one tree into another, rename branch, switch branches with each other

delete, insert, create, update
