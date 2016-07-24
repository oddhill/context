<?php

namespace Drupal\context_layout\Form;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Manage module configuration form.
 */
class SettingsForm extends ConfigFormBase {

  /**
   * Constructs SettingsForm object.
   *
   * @param \Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface $contextLayoutManager
   *    Context layout manager service.
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *    Config factory service.
   */
  public function __construct(LayoutPluginManagerInterface $contextLayoutManager, ConfigFactoryInterface $config_factory) {
    $this->contextLayoutManager = $contextLayoutManager;
    parent::__construct($config_factory);
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'context_layout_settings';
  }

  /**
   * Inject dependencies to constructor.
   *
   * @param \Symfony\Component\DependencyInjection\ContainerInterface $container
   *    The current service container.
   *
   * @return static
   *    Dependencies for injection.
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('plugin.manager.context_layout'),
      $container->get('config.factory')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // Module settings.
    $config = $this->config('context_layout.settings');

    // Default layout for layout reactions.
    $form['default_layout'] = [
      '#title' => t('Default Layout'),
      '#description' => t('Select a default layout for layout reactions'),
      '#type' => 'select',
      '#options' => [0 => '---'] + $this->contextLayoutManager->getLayoutOptions(
          ['group_by_category' => TRUE]
        ),
      '#default_value' => $config->get('default_layout'),
    ];

    // Exclude layout displays on admin routes.
    $form['admin_allow'] = [
      '#title' => t('Allow Admin Routes'),
      '#description' => t('Check to allow Context Layout reactions on admin routes, eg: admin/structure.'),
      '#type' => 'checkbox',
      '#default_value' => $config->get('admin_allow'),
    ];

    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();
    $this->config('context_layout.settings')
      ->set('default_layout', $values['default_layout'])
      ->set('admin_allow', $values['admin_allow'])
      ->save();
    parent::submitForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['context_layout.settings'];
  }

}
