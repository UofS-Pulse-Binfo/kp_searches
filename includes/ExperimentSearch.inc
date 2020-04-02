<?php

/**
 * Provides a search for chado featuremap-based Tripal Content.
 *
 */
class ExperimentSearch extends KPSEARCH {

  /**
   * The human readable title of this search. This will be used in listings
   * and shown on the search page as the title.
   */
  public static $title = 'Experiment Search';

  /**
   * The machine name of the module providing this search.
   */
  public static $module = 'kp_searches';

  /**
   * A description of this search. This is shown at the top of the search page
   * and used for the menu item.
   */
  public static $description = '';

  /**
   * The machine names of the permissions with access to this search. This is
   * used to map your search to existing permissions. It must be an array and
   * is used in hook_menu(). Some examples include 'access content'
   * and 'administer tripal'.
   */
  public static $permissions = ['access content'];

  /**
   * If TRUE, this search will require the submit button to be clicked before
   * executing the query; whereas, if FALSE it will be executed on the
   * first page load without parameters.
   *
   * NOTE: to control the results without parameters check $this->submitted
   * in getQuery() and if FALSE return your pre-submit query.
   */
  public static $require_submit = FALSE;

  /**
   * Add a pager to search results
   * with $num_items_per_page results on a single page.
   * NOTE: Your query has to handle paging.
   */
  public static $pager = TRUE;
  public static $num_items_per_page = 50;

  /**
   * This defined the hook_menu definition for this search. At a minimum the
   * path is required.
   */
  public static $menu = [
    'path' => 'research/projects',
    // @todo support menu items.
  ];

  /**
   * Add CSS/JS to the form/results page.
   * NOTE: paths supplied should be relative to $module.
   */
  public static $attached = [
    'css' => [
      'css/all_kp_searches.css',
    ],
    'js' => [],
  ];

  /**
   * Information regarding the fields and filters for this search.
   */
  public static $info = [
    // Lists the columns in your results table.
    'fields' => [
      'name' => [
        'title' => 'Name',
        'entity_link' => [
          'chado_table' => 'project',
          'id_column' => 'project_id'
        ],
      ],
      'genus' => [
        'title' => 'Genus',
      ],
      'year' => [
        'title' => 'Dates',
      ],
      'category' => [
        'title' => 'Category',
      ],
    ],
    // The filter criteria available to the user.
    // This is used to generate a search form which can be altered.
    'filters' => [
      'genus' => [
        'title' => 'Genus',
        'help' => 'The legume species the experiment is focused on (e.g. Lens culinaris).',
        'default' => 'Lens',
      ],
      'species' => [
        'title' => 'Species',
        'help' => '',
      ],
      'year' => [
        'title' => 'Year',
        'help' => 'The years the project is ongoing for.',
      ],
      'category' => [
        'title' => 'Category',
        'help' => 'The category of the experiment.',
      ],
      'name' => [
        'title' => 'Name',
        'help' => 'The name of the experiment you are interested in (partial names are accepted).',
      ],
    ],
  ];

  /**
   * Text that should appear on the button at the bottom of the importer
   * form.
   */
  public static $button_text = 'Search';

  /**
   * Generate the filter form.
   *
   * The base class will generate textfields for each filter defined in $info,
   * set defaults, labels and descriptions, as well as, create the search
   * button.
   *
   * Extend this method to alter the filter form.
   */
  public function form($form, $form_state) {
    $form = parent::form($form, $form_state);

    // Add a crop selector.
    $this->addCropFormElement($form, $form_state);

    // Add a Scientific name selector.
    $this->addSpeciesFormElement($form, $form_state);

    // @todo: add species to the database in a consistent manner.
    $form['scientific_name']['species']['#disabled'] = TRUE;

    return $form;
  }

  /**
   * Allows custom searches to validate the form results.
   * Use form_set_error() to signal invalid values.
   */
  public function validateForm($form, $form_state) {
    //$values = $form_state['values'];
  }

  /**
   * Determine the query for the genetic marker search.
   *
   * @param string $query
   *   The full SQL query to execute. This will be executed using chado_query()
   *   so use curly brackets appropriately. Use :placeholders for any values.
   * @param array $args
   *   An array of arguments to pass to chado_query(). Keys must be the
   *   placeholders in the query and values should be what you want them set to.
   */
  public function getQuery(&$query, &$args, $offset) {

    // Retrieve the filter results already set.
    $filter_results = $this->values;
    // @debug dpm($filter_results, '$filter_results');

    // @todo Collaborators: 5311
    $query = "
      SELECT
        exp.name, exp.project_id,
        exp.description,
        genus.value as genus,
        yr.value as year,
        cat.value as category
      FROM {project} exp
      LEFT JOIN {projectprop} genus ON genus.project_id=exp.project_id
        AND genus.type_id IN (SELECT cvterm_id FROM {cvterm} WHERE name='genus')
      LEFT JOIN {projectprop} yr ON yr.project_id=exp.project_id
        AND yr.type_id=6364 AND yr.rank=0
      LEFT JOIN {projectprop} cat ON cat.project_id=exp.project_id
        AND cat.type_id=6365 AND cat.rank=0
      LEFT JOIN {projectprop} re ON re.project_id=exp.project_id
        AND re.type_id=4310 AND re.rank=0";

    $where = [];
    $joins = [];

    $where[] = "re.value = :content_type";
    $args[':content_type'] = 'Research Experiment';
    // -- Genus.
    if ($filter_results['genus'] != '') {
      $where[] = "genus.value ~* :genus";
      $args[':genus'] = $filter_results['genus'];
    }

    // -- Species.
    if ($filter_results['species'] != '') {
      $where[] = "species.value ~* :species";
      $args[':species'] = $filter_results['species'];
    }

    // -- Name.
    if ($filter_results['name'] != '') {
      $where[] = "exp.name ~ :name";
      $args[':name'] = $filter_results['name'];
    }

    // Finally, add it to the query.
    if (!empty($joins)) {
      $query .= implode("\n",$joins);
    }
    if (!empty($where)) {
      $query .= "\n" . ' WHERE ' . implode(' AND ',$where);
    }

    // Sort even though it is SLOW b/c ppl expect it.
    $query .= "\n" . ' ORDER BY exp.name ASC';

    // Handle paging.
    $limit = $this::$num_items_per_page + 1;
    $query .= "\n" . ' LIMIT ' . $limit . ' OFFSET ' . $offset;

    // @debug dpm(strtr(str_replace(['{','}'], ['chado.', ''], $query), $args), 'query');
  }

  /**
   * Format the results within the $form array.
   *
   * The base class will format the results as a table.
   *
   * @param array $form
   *   The current form array.
   * @param array $results
   *   The results to format. This will be an array of standard objects where
   *   the keys map to the keys in $info['fields'].
   */
  public function formatResults(&$form, $results) {
    $list = [];

    foreach ($results as $r) {
      // Add a link to the title.
      $title = $r->name;
      $id = NULL;
      if ($r->project_id) {
        $id = chado_get_record_entity_by_table('project', $r->project_id);
        if ($id) {
          $title = l($r->name, 'bio_data/'.$id);
        }
      }

      // Substring the description.
      $description = strip_tags($r->description);
      if (preg_match('/^.{1,500}\b/s', $description, $match)) {
        $description = trim($match[0]);
      }
      if ($id) {
        $description .= ' ' . l('[Read more]', 'bio_data/'.$id);
      }

      $markup = '
        <div class="result-row">
          <div class="result-left">
            <div class="result-title">'.$title.'</div>
            <div class="result-description">'.$description.'</div>
          </div>
          <div class="result-right">
            <div class="result-year">'.$r->year.'</div>
            <div class="result-genus">'.$r->genus.'</div>
            <div class="result-category">'.$r->category.'</div>
          </div>
        </div>';
      $list[] = $markup;
    }

    $form['results'] = [
      '#theme' => 'item_list',
      '#items' => $list,
      '#type' => 'ul',
      '#weight' => 50,
    ];

    return $form;
  }
}
